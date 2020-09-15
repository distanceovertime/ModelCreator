
#include "Terrain_Texture.h"

// global variables
FbxManager* lManager = NULL;
FbxScene* gScene = NULL;
FbxFileTexture* gTexture = NULL;
FbxSurfacePhong* gMaterial = NULL;

#ifdef IOS_REF
	#undef  IOS_REF
	#define IOS_REF (*(lManager->GetIOSettings()))
#endif

bool SaveScene(FbxManager* pManager,
	FbxScene* pScene,
	const char* pFilename,
	int pFileFormat,
	bool pEmbedMedia)
{
	int lMajor, lMinor, lRevision;
	bool lStatus = true;

	// Create exporter
	FbxExporter* lExporter = FbxExporter::Create(pManager, "");

	if (pFileFormat < 0 || pFileFormat >= pManager->GetIOPluginRegistry()->GetWriterFormatCount())
	{
		// Write in fall back format if pEmbedMedia is true
		// ?????
		pFileFormat = pManager->GetIOPluginRegistry()->GetNativeWriterFormat();

		if (!pEmbedMedia)
		{
			// Try to export in ASCII if possible
			int lFormatIndex, lFormatCount = pManager->GetIOPluginRegistry()->GetWriterFormatCount();

			for (lFormatIndex = 0; lFormatIndex < lFormatCount; lFormatIndex++)
			{
				if (pManager->GetIOPluginRegistry()->
					WriterIsFBX(lFormatIndex))
				{
					FbxString lDesc = pManager->GetIOPluginRegistry()->
						GetWriterFormatDescription(lFormatIndex);
					if (lDesc.Find("ascii") >= 0)
					{
						pFileFormat = lFormatIndex;
						break;
					}
				}
			}

			// Initialize the exporter
			if (lExporter->Initialize(pFilename, pFileFormat, pManager->GetIOSettings()) == false)
			{
				printf("Call to FbxExporter::Initialize() failed.");
				printf("Error returned: %s", lExporter->GetStatus().GetErrorString());
				return false;
			}

			FbxManager::GetFileFormatVersion(lMajor, lMinor, lRevision);

			printf("FBX version number for this FBX SDK is %d.%d.%d \n",
				lMajor, lMinor, lRevision);

			/* if (pManager->GetIOPluginRegistry()->WriterIsFBX(pFileFormat))
			{
				// Export options determine what kind of data is to be imported.
				// The default (except for the option eEXPORT_TEXTURE_AS_EMBEDDED)
				// is true, but here we set the options explicitly.
				IOS_REF.SetBoolProp(EXP_FBX_MATERIAL, true);
				IOS_REF.SetBoolProp(EXP_FBX_TEXTURE, true);
				IOS_REF.SetBoolProp(EXP_FBX_EMBEDDED, pEmbedMedia);
				IOS_REF.SetBoolProp(EXP_FBX_SHAPE, true);
				IOS_REF.SetBoolProp(EXP_FBX_GOBO, true);
				IOS_REF.SetBoolProp(EXP_FBX_ANIMATION, true);
				IOS_REF.SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);
			} **/

			// Export the scene.
			lStatus = lExporter->Export(pScene);

			// Destroy the exporter.
			lExporter->Destroy();

			return lStatus;
		}
	}
}

void CreateTexture(FbxScene* pScene)
{
	gTexture = FbxFileTexture::Create(pScene, "Diffuse Texture");

	FbxString lTexPath = "C:\\Users\\Thomas Twist\\source\\repos\\Terrain_Texture\\Debug\\texture4.jpg";

	gTexture->SetFileName(lTexPath.Buffer());
	gTexture->SetTextureUse(FbxTexture::eStandard);
	gTexture->SetMappingType(FbxTexture::eUV);
	gTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
	gTexture->SetSwapUV(false);
	gTexture->SetTranslation(0.0, 0.0);
	gTexture->SetScale(1.0, 1.0);
	gTexture->SetRotation(0.0, 0.0);
}


void CreateMaterial(FbxScene* pScene)
{
	FbxString lMaterialName = "material";
	FbxString lShadingName = "Phong";
	FbxDouble3 lBlack(0.0, 0.0, 0.0);
	FbxDouble3 lRed(1.0, 0.0, 0.0);
	FbxDouble3 lDiffuseColor(0.75, 0.75, 0.0);
	gMaterial = FbxSurfacePhong::Create(pScene, lMaterialName.Buffer());

	// Generate primary and secondary colors.
	gMaterial->Emissive.Set(lBlack);
	gMaterial->Ambient.Set(lRed);
	gMaterial->Diffuse.Set(lDiffuseColor);
	gMaterial->TransparencyFactor.Set(40.5);
	gMaterial->ShadingModel.Set(lShadingName);
	gMaterial->Shininess.Set(0.5);

	// the texture need to be connected to the material on the corresponding property (in this case Diffuse)
	if (gTexture)
		gMaterial->Diffuse.ConnectSrcObject(gTexture);
}

void AddMaterials(FbxMesh* pMesh)
{
	// load UV information from mesh
	
	// get all UV set names
	FbxStringList lUVSetNameList;
	pMesh->GetUVSetNames(lUVSetNameList);

	printf("get count %u \n", lUVSetNameList.GetCount());

	// Set material mapping.
	FbxGeometryElementMaterial* lMaterialElement = pMesh->CreateElementMaterial();
	lMaterialElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
	lMaterialElement->SetReferenceMode(FbxGeometryElement::eDirect);

	// iterate over all UV sets
	// terrain only has one UV set
	for (int lUVSetIndex = 0; lUVSetIndex < lUVSetNameList.GetCount(); lUVSetIndex++)
	{
		// get lUVSetIndex-th UV set
		// lUVElement is an array of our UV elements
		const char* lUVSetName = lUVSetNameList.GetStringAt(lUVSetIndex);
		const FbxGeometryElementUV* lUVElement = pMesh->GetElementUV(lUVSetName);

		if (!lUVElement)
			continue;

		// only support mapping mode eByPolygonVertex and eByControlPoint
		if (lUVElement->GetMappingMode() != FbxGeometryElement::eByPolygonVertex &&
			lUVElement->GetMappingMode() != FbxGeometryElement::eByControlPoint)
			return;

		// index array to hold the index referenced to uv data
		// lUseIndex returns true for both eIndexToDirect and eIndex
		// const bool lUseIndex = lUVElement->GetReferenceMode() != FbxGeometryElement::eDirect;
		const bool lUseIndex = true;
		const int lIndexCount = (lUseIndex) ? lUVElement->GetIndexArray().GetCount() : 0;
		printf("Index count: %i \n", lIndexCount);
		

		// iterate through the data by polygon
		// 1 polygon = 4 indexes
		const int lPolyCount = pMesh->GetPolygonCount();

		if (lUVElement->GetMappingMode() == FbxGeometryElement::eByEdge) 
		{
			for (int lPolyIndex = 0; lPolyIndex < lPolyCount; ++lPolyIndex) 
			{
				// build the max index array that we need to pass into MakePoly
				const int lPolySize = pMesh->GetPolygonSize(lPolyIndex);
				for (int lVertIndex = 0; lVertIndex < lPolySize; ++lVertIndex)
				{
					FbxVector2 lUVValue;

					// get the index of the current vertex in control points array
					int lPolyVertIndex = pMesh->GetPolygonVertex(lPolyIndex, lVertIndex);

					int lUVIndex = lUseIndex ? lUVElement->GetIndexArray().GetAt(lPolyVertIndex) : lPolyVertIndex;

					lUVValue = lUVElement->GetDirectArray().GetAt(lUVIndex);

					printf("%d", lUVValue.Length());
				}
			}
		}
		else if (lUVElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
		{
			int lPolyIndexCounter = 0;
			for (int lPolyIndex = 0; lPolyIndex < lPolyCount; ++lPolyIndex)
			{
				// build the max index array 
				const int lPolySize = pMesh->GetPolygonSize(lPolyIndex);
				// printf("lPolySize: %i", lPolySize); #always 4
				for (int lVertIndex = 0; lVertIndex < lPolySize; ++lVertIndex)
				{
					if (lPolyIndexCounter < lIndexCount)
					{
						FbxVector2 lUVValue;

						//the UV index depends on the reference mode
						int lUVIndex = lUseIndex ? lUVElement->GetIndexArray().GetAt(lPolyIndexCounter) : lPolyIndexCounter;

						lUVValue = lUVElement->GetDirectArray().GetAt(lUVIndex);

						// EXPERIMENTAL: mess with UV values
						//FbxVector2 modified = FbxVector2(lUVElement->GetDirectArray().GetAt(lUVIndex).mData[0] - 1000, lUVElement->GetDirectArray().GetAt(lUVIndex).mData[1] - 1000);
						//lUVElement->GetDirectArray().SetAt(lUVIndex, modified);

						//User TODO:
						//Print out the value of UV(lUVValue) or log it to a file

						printf("Vertex from this polygon: %i ", lPolyIndex);
						printf("UV data for this vertex: %i %i \n", lUVElement->GetDirectArray().GetAt(lUVIndex).mData[0], lUVElement->GetDirectArray().GetAt(lUVIndex).mData[1]);

						lPolyIndexCounter++;
					}

				}
			}
			printf("Poly index counter: %i \n", lPolyIndexCounter);
		}
	}
	
	
	

	//get the node of mesh, add material for it.
	FbxNode* lNode = pMesh->GetNode();
	if (lNode == NULL)
	{
		return;
	}
	lNode->AddMaterial(gMaterial);

	// We are in eByPolygon, so there's only need for 6 index (a cube has 6 polygons).
	lMaterialElement->GetIndexArray().SetCount(pMesh->GetPolygonCount());

	// Set the Index 0 to 6 to the material in position 0 of the direct array.
	for (int i = 0; i < pMesh->GetPolygonCount(); ++i)
		lMaterialElement->GetIndexArray().SetAt(i, 0);

	//iterating over all uv sets
	
}

// main
int main(int argc, char** argv)
{
	FbxManager* lManager = FbxManager::Create();
	FBXSDK_printf("Autodesk FBX SDK version %s\n", lManager->GetVersion());


	// Create an IOSettings object to hold import/export settings
	// IOSROOT = "IOSRoot"
	FbxIOSettings* ios = FbxIOSettings::Create(lManager, IOSROOT);
	lManager->SetIOSettings(ios);

	// create an importer
	FbxImporter* lImporter = FbxImporter::Create(lManager, "");

	// use first argument as file name for importer
	// TODO: shouldn't hard code this
	if (!lImporter->Initialize("C:\\Users\\Thomas Twist\\source\\repos\\Terrain_Texture\\Debug\\terrain2.fbx", -1, lManager->GetIOSettings()))
	{
		printf("Failure when initializing importer \n");
		printf("Error returned: %s\n\n", lImporter->GetStatus().GetErrorString());
		exit(-1);
	}

	FbxScene* lScene = FbxScene::Create(lManager, "My Scene");

	lImporter->Import(lScene); 

	lImporter->Destroy();

	// Apply texture to mesh
	CreateTexture(lScene);

	CreateMaterial(lScene);

	// Grab terrain mesh from scene
	FbxMesh* lMesh = lScene->GetRootNode()->GetChild(0)->GetMesh();

	FbxNode* terrainNode = lScene->GetRootNode()->GetChild(0);

	int count = lScene->GetRootNode()->GetChildCount();

	if (lMesh->GetPolygonCount() > 0)
	{
		printf("Node visible, polygon count of %i \n", lMesh->GetPolygonCount());
	}
	else {
		printf("Node not visible \n");
	}
	

	AddMaterials(lMesh);

	// Export Scene

	bool r;

	r = SaveScene(lManager, lScene, "C:\\Users\\Thomas Twist\\source\\repos\\Terrain_Texture\\Debug\\terrainexported6.fbx", -1, false);
	if (r)
	{
		printf("Export succeeded \n");
	}
	else 
	{
		printf("Export failed");
	}
	


}





bool InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
{
	// create FBX Manager 
	pManager = FbxManager::Create();

	// check for error in creation, otherwise print FBX SDK version to signal success
	if (!pManager)
	{
		FBXSDK_printf("Error: Unable to create FBX Manager!\n");
		exit(1);
	}
	else FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());

	// Create an IOSettings object to hold import/export settings
	// IOSROOT = "IOSRoot"
	FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
	pManager->SetIOSettings(ios);


	// Create an FBX scene and check for success
	pScene = FbxScene::Create(pManager, "Main Scene");
	if (!pScene)
	{
		FBXSDK_printf("Error in creating FBX Scene");
		exit(1);
	}
	else FBXSDK_printf("Scene created!");

	return true;
}

