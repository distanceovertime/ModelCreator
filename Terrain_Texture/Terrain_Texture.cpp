
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

	FbxString lTexPath = "C:\\Users\\thoma\\modelcreator\\ModelCreator\\Debug\\texture.jpg";

	gTexture->SetFileName(lTexPath.Buffer());
	gTexture->SetTextureUse(FbxTexture::eStandard);
	gTexture->SetMappingType(FbxTexture::eUV);
	gTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
	gTexture->SetSwapUV(false);
	gTexture->SetTranslation(0.0, 0.0);
	gTexture->SetScale(1.0, 1.0);
	gTexture->SetRotation(0.0, 0.0);
}

// Create texture for cube.




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

	// printf("get count %u \n", lUVSetNameList.GetCount());

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
		// printf("Index count: %i \n", lIndexCount);
		

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

					// printf("%d", lUVValue.Length());
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

						// printf("Vertex from this polygon: %i ", lPolyIndex);
						// printf("UV data for this vertex: %i %i \n", lUVElement->GetDirectArray().GetAt(lUVIndex).mData[0], lUVElement->GetDirectArray().GetAt(lUVIndex).mData[1]);

						lPolyIndexCounter++;
					}

				}
			}
			// printf("Poly index counter: %i \n", lPolyIndexCounter);
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

FbxScene* CreateScene()
{
	// Create the SDK manager.
	FbxManager* lSdkManager = FbxManager::Create();

	// Create the scene.
	FbxScene* lScene = FbxScene::Create(lSdkManager, "Created_Scene");

	printf("Scene Created \n");

	return lScene;
}

// Create materials for pyramid.
void CreateMaterials(FbxScene* pScene, FbxMesh* pMesh)
{
	int i;

	for (i = 0; i < 5; i++)
	{
		FbxString lMaterialName = "material";
		FbxString lShadingName = "Phong";
		lMaterialName += i;
		FbxDouble3 lBlack(0.0, 0.0, 0.0);
		FbxDouble3 lRed(1.0, 0.0, 0.0);
		FbxDouble3 lWhite(1.0, 1.0, 1.0);
		FbxDouble3 lColor;
		FbxSurfacePhong* lMaterial = FbxSurfacePhong::Create(pScene, lMaterialName.Buffer());


		// Generate primary and secondary colors.
		lMaterial->Emissive.Set(lBlack);
		lMaterial->Ambient.Set(lWhite);
		lColor = FbxDouble3(i > 2 ? 1.0 : 0.0,
			i > 0 && i < 4 ? 1.0 : 0.0,
			i % 2 ? 0.0 : 1.0);
		lMaterial->Diffuse.Set(lColor);
		lMaterial->TransparencyFactor.Set(0.0);
		lMaterial->ShadingModel.Set(lShadingName);
		lMaterial->Shininess.Set(0.5);

		//get the node of mesh, add material for it.
		FbxNode* lNode = pMesh->GetNode();
		if (lNode)
			lNode->AddMaterial(lMaterial);
		//if (gTexture)
		//	lMaterial->Diffuse.ConnectSrcObject(gTexture);
	}
	
}

// Create texture for cube.
void CreateCubeTexture(FbxScene* pScene, FbxMesh* pMesh)
{
	// A texture need to be connected to a property on the material,
	// so let's use the material (if it exists) or create a new one
	FbxSurfacePhong* lMaterial = NULL;

	//get the node of mesh, add material for it.
	FbxNode* lNode = pMesh->GetNode();
	if (lNode)
	{
		lMaterial = lNode->GetSrcObject<FbxSurfacePhong>(0);
		if (lMaterial == NULL)
		{
			FbxString lMaterialName = "toto";
			FbxString lShadingName = "Phong";
			FbxDouble3 lBlack(0.0, 0.0, 0.0);
			FbxDouble3 lRed(1.0, 0.0, 0.0);
			FbxDouble3 lDiffuseColor(0.75, 0.75, 0.0);
			lMaterial = FbxSurfacePhong::Create(pScene, lMaterialName.Buffer());

			// Generate primary and secondary colors.
			lMaterial->Emissive.Set(lBlack);
			lMaterial->Ambient.Set(lRed);
			lMaterial->AmbientFactor.Set(1.);
			// Add texture for diffuse channel
			lMaterial->Diffuse.Set(lDiffuseColor);
			lMaterial->DiffuseFactor.Set(1.);
			lMaterial->TransparencyFactor.Set(0.4);
			lMaterial->ShadingModel.Set(lShadingName);
			lMaterial->Shininess.Set(0.5);
			lMaterial->Specular.Set(lBlack);
			lMaterial->SpecularFactor.Set(0.3);

			lNode->AddMaterial(lMaterial);
		}
	}

	FbxFileTexture* lTexture = FbxFileTexture::Create(pScene, "Diffuse Texture");

	// Set texture properties.
	lTexture->SetFileName("texture.jpg"); // Resource file is in current directory.
	lTexture->SetTextureUse(FbxTexture::eStandard);
	lTexture->SetMappingType(FbxTexture::eUV);
	lTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
	lTexture->SetSwapUV(false);
	lTexture->SetTranslation(0.0, 0.0);
	lTexture->SetScale(1.0, 1.0);
	lTexture->SetRotation(0.0, 0.0);

	// don't forget to connect the texture to the corresponding property of the material
	if (lMaterial)
		lMaterial->Diffuse.ConnectSrcObject(lTexture);

	lTexture = FbxFileTexture::Create(pScene, "Ambient Texture");

	// Set texture properties.
	lTexture->SetFileName("white2.jpg"); // Resource file is in current directory.
	lTexture->SetTextureUse(FbxTexture::eStandard);
	lTexture->SetMappingType(FbxTexture::eUV);
	lTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
	lTexture->SetSwapUV(false);
	lTexture->SetTranslation(0.0, 0.0);
	lTexture->SetScale(1.0, 1.0);
	lTexture->SetRotation(0.0, 0.0);

	// don't forget to connect the texture to the corresponding property of the material
	if (lMaterial)
		lMaterial->Ambient.ConnectSrcObject(lTexture);

	lTexture = FbxFileTexture::Create(pScene, "Emissive Texture");

	// Set texture properties.
	lTexture->SetFileName("texture.jpg"); // Resource file is in current directory.
	lTexture->SetTextureUse(FbxTexture::eStandard);
	lTexture->SetMappingType(FbxTexture::eUV);
	lTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
	lTexture->SetSwapUV(false);
	lTexture->SetTranslation(0.0, 0.0);
	lTexture->SetScale(1.0, 1.0);
	lTexture->SetRotation(0.0, 0.0);

	// don't forget to connect the texture to the corresponding property of the material
	if (lMaterial)
		lMaterial->Emissive.ConnectSrcObject(lTexture);
}

FbxNode* CreateMesh(FbxString mType, FbxScene* mScene)
{
	printf("Creating Mesh... \n");
	if (mType.Compare("Pyramid") == 0)
	{
		printf("Creating Preset: Pyramid \n");
		int i, j;
		FbxMesh* lMesh = FbxMesh::Create(mScene, "CreatedPyramidMesh");

		FbxVector4 vertex0(-50, 0, 50);
		FbxVector4 vertex1(50, 0, 50);
		FbxVector4 vertex2(50, 0, -50);
		FbxVector4 vertex3(-50, 0, -50);
		FbxVector4 vertex4(0, 100, 0);

		FbxVector4 lNormalP0(0, 1, 0);
		FbxVector4 lNormalP1(0, 0.447, 0.894);
		FbxVector4 lNormalP2(0.894, 0.447, 0);
		FbxVector4 lNormalP3(0, 0.447, -0.894);
		FbxVector4 lNormalP4(-0.894, 0.447, 0);

		// Create control points.
		lMesh->InitControlPoints(16);
		FbxVector4* lControlPoints = lMesh->GetControlPoints();

		lControlPoints[0] = vertex0;
		lControlPoints[1] = vertex1;
		lControlPoints[2] = vertex2;
		lControlPoints[3] = vertex3;
		lControlPoints[4] = vertex0;
		lControlPoints[5] = vertex1;
		lControlPoints[6] = vertex4;
		lControlPoints[7] = vertex1;
		lControlPoints[8] = vertex2;
		lControlPoints[9] = vertex4;
		lControlPoints[10] = vertex2;
		lControlPoints[11] = vertex3;
		lControlPoints[12] = vertex4;
		lControlPoints[13] = vertex3;
		lControlPoints[14] = vertex0;
		lControlPoints[15] = vertex4;

		// specify normals per control point.

		FbxGeometryElementNormal* lNormalElement = lMesh->CreateElementNormal();
		lNormalElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
		lNormalElement->SetReferenceMode(FbxGeometryElement::eDirect);

		lNormalElement->GetDirectArray().Add(lNormalP0);
		lNormalElement->GetDirectArray().Add(lNormalP0);
		lNormalElement->GetDirectArray().Add(lNormalP0);
		lNormalElement->GetDirectArray().Add(lNormalP0);
		lNormalElement->GetDirectArray().Add(lNormalP1);
		lNormalElement->GetDirectArray().Add(lNormalP1);
		lNormalElement->GetDirectArray().Add(lNormalP1);
		lNormalElement->GetDirectArray().Add(lNormalP2);
		lNormalElement->GetDirectArray().Add(lNormalP2);
		lNormalElement->GetDirectArray().Add(lNormalP2);
		lNormalElement->GetDirectArray().Add(lNormalP3);
		lNormalElement->GetDirectArray().Add(lNormalP3);
		lNormalElement->GetDirectArray().Add(lNormalP3);
		lNormalElement->GetDirectArray().Add(lNormalP4);
		lNormalElement->GetDirectArray().Add(lNormalP4);
		lNormalElement->GetDirectArray().Add(lNormalP4);


		// Array of polygon vertices.
		int lPolygonVertices[] = { 0, 3, 2, 1,
			4, 5, 6,
			7, 8, 9,
			10, 11, 12,
			13, 14, 15 };

		// Set material mapping.
		FbxGeometryElementMaterial* lMaterialElement = lMesh->CreateElementMaterial();
		lMaterialElement->SetMappingMode(FbxGeometryElement::eByPolygon);
		lMaterialElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

		// Create polygons. Assign material indices.

		// Pyramid base.
		lMesh->BeginPolygon(0); // Material index.

		for (j = 0; j < 4; j++)
		{
			lMesh->AddPolygon(lPolygonVertices[j]); // Control point index.
		}

		lMesh->EndPolygon();

		// Pyramid sides.
		for (i = 1; i < 5; i++)
		{
			lMesh->BeginPolygon(i); // Material index.

			for (j = 0; j < 3; j++)
			{
				lMesh->AddPolygon(lPolygonVertices[4 + 3 * (i - 1) + j]); // Control point index.
			}

			lMesh->EndPolygon();
		}


		FbxNode* lNode = FbxNode::Create(mScene, "Created_Node");

		lNode->SetNodeAttribute(lMesh);

		// TO DO: Adjust for Pyramids
		CreateMaterials(mScene, lMesh);

		return lNode;
	}
	else if (mType.Compare("Cube") == 0) 
	{
		printf("Creating Preset: Cube \n");
		int i, j;
		FbxMesh* lMesh = FbxMesh::Create(mScene, "Textured_Cube");

		FbxVector4 vertex0(-50, 0, 50);
		FbxVector4 vertex1(50, 0, 50);
		FbxVector4 vertex2(50, 100, 50);
		FbxVector4 vertex3(-50, 100, 50);
		FbxVector4 vertex4(-50, 0, -50);
		FbxVector4 vertex5(50, 0, -50);
		FbxVector4 vertex6(50, 100, -50);
		FbxVector4 vertex7(-50, 100, -50);

		FbxVector4 lNormalXPos(1, 0, 0);
		FbxVector4 lNormalXNeg(-1, 0, 0);
		FbxVector4 lNormalYPos(0, 1, 0);
		FbxVector4 lNormalYNeg(0, -1, 0);
		FbxVector4 lNormalZPos(0, 0, 1);
		FbxVector4 lNormalZNeg(0, 0, -1);

		// Create control points.
		lMesh->InitControlPoints(24);
		FbxVector4* lControlPoints = lMesh->GetControlPoints();

		lControlPoints[0] = vertex0;
		lControlPoints[1] = vertex1;
		lControlPoints[2] = vertex2;
		lControlPoints[3] = vertex3;
		lControlPoints[4] = vertex1;
		lControlPoints[5] = vertex5;
		lControlPoints[6] = vertex6;
		lControlPoints[7] = vertex2;
		lControlPoints[8] = vertex5;
		lControlPoints[9] = vertex4;
		lControlPoints[10] = vertex7;
		lControlPoints[11] = vertex6;
		lControlPoints[12] = vertex4;
		lControlPoints[13] = vertex0;
		lControlPoints[14] = vertex3;
		lControlPoints[15] = vertex7;
		lControlPoints[16] = vertex3;
		lControlPoints[17] = vertex2;
		lControlPoints[18] = vertex6;
		lControlPoints[19] = vertex7;
		lControlPoints[20] = vertex1;
		lControlPoints[21] = vertex0;
		lControlPoints[22] = vertex4;
		lControlPoints[23] = vertex5;


		// We want to have one normal for each vertex (or control point),
		// so we set the mapping mode to eBY_CONTROL_POINT.
		FbxGeometryElementNormal* lGeometryElementNormal = lMesh->CreateElementNormal();

		lGeometryElementNormal->SetMappingMode(FbxGeometryElement::eByControlPoint);
		printf("here \n");
		// Here are two different ways to set the normal values.
		bool firstWayNormalCalculations = false;
		if (firstWayNormalCalculations)
		{
			// The first method is to set the actual normal value
			// for every control point.
			lGeometryElementNormal->SetReferenceMode(FbxGeometryElement::eDirect);

			lGeometryElementNormal->GetDirectArray().Add(lNormalZPos);
			lGeometryElementNormal->GetDirectArray().Add(lNormalZPos);
			lGeometryElementNormal->GetDirectArray().Add(lNormalZPos);
			lGeometryElementNormal->GetDirectArray().Add(lNormalZPos);
			lGeometryElementNormal->GetDirectArray().Add(lNormalXPos);
			lGeometryElementNormal->GetDirectArray().Add(lNormalXPos);
			lGeometryElementNormal->GetDirectArray().Add(lNormalXPos);
			lGeometryElementNormal->GetDirectArray().Add(lNormalXPos);
			lGeometryElementNormal->GetDirectArray().Add(lNormalZNeg);
			lGeometryElementNormal->GetDirectArray().Add(lNormalZNeg);
			lGeometryElementNormal->GetDirectArray().Add(lNormalZNeg);
			lGeometryElementNormal->GetDirectArray().Add(lNormalZNeg);
			lGeometryElementNormal->GetDirectArray().Add(lNormalXNeg);
			lGeometryElementNormal->GetDirectArray().Add(lNormalXNeg);
			lGeometryElementNormal->GetDirectArray().Add(lNormalXNeg);
			lGeometryElementNormal->GetDirectArray().Add(lNormalXNeg);
			lGeometryElementNormal->GetDirectArray().Add(lNormalYPos);
			lGeometryElementNormal->GetDirectArray().Add(lNormalYPos);
			lGeometryElementNormal->GetDirectArray().Add(lNormalYPos);
			lGeometryElementNormal->GetDirectArray().Add(lNormalYPos);
			lGeometryElementNormal->GetDirectArray().Add(lNormalYNeg);
			lGeometryElementNormal->GetDirectArray().Add(lNormalYNeg);
			lGeometryElementNormal->GetDirectArray().Add(lNormalYNeg);
			lGeometryElementNormal->GetDirectArray().Add(lNormalYNeg);
		}
		else
		{
			// The second method is to the possible values of the normals
			// in the direct array, and set the index of that value
			// in the index array for every control point.
			lGeometryElementNormal->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

			// Add the 6 different normals to the direct array
			lGeometryElementNormal->GetDirectArray().Add(lNormalZPos);
			lGeometryElementNormal->GetDirectArray().Add(lNormalXPos);
			lGeometryElementNormal->GetDirectArray().Add(lNormalZNeg);
			lGeometryElementNormal->GetDirectArray().Add(lNormalXNeg);
			lGeometryElementNormal->GetDirectArray().Add(lNormalYPos);
			lGeometryElementNormal->GetDirectArray().Add(lNormalYNeg);

			// Now for each control point, we need to specify which normal to use
			lGeometryElementNormal->GetIndexArray().Add(0); // index of lNormalZPos in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(0); // index of lNormalZPos in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(0); // index of lNormalZPos in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(0); // index of lNormalZPos in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(1); // index of lNormalXPos in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(1); // index of lNormalXPos in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(1); // index of lNormalXPos in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(1); // index of lNormalXPos in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(2); // index of lNormalZNeg in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(2); // index of lNormalZNeg in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(2); // index of lNormalZNeg in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(2); // index of lNormalZNeg in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(3); // index of lNormalXNeg in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(3); // index of lNormalXNeg in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(3); // index of lNormalXNeg in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(3); // index of lNormalXNeg in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(4); // index of lNormalYPos in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(4); // index of lNormalYPos in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(4); // index of lNormalYPos in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(4); // index of lNormalYPos in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(5); // index of lNormalYNeg in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(5); // index of lNormalYNeg in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(5); // index of lNormalYNeg in the direct array.
			lGeometryElementNormal->GetIndexArray().Add(5); // index of lNormalYNeg in the direct array.


			 // Array of polygon vertices.
			int lPolygonVertices[] = { 0, 1, 2, 3,
				4, 5, 6, 7,
				8, 9, 10, 11,
				12, 13, 14, 15,
				16, 17, 18, 19,
				20, 21, 22, 23 };

			// Create UV for Diffuse channel
			FbxGeometryElementUV* lUVDiffuseElement = lMesh->CreateElementUV("DiffuseUV");
			if (lUVDiffuseElement == NULL)
			{
				printf("Error: Wrong Diffuse Element");
				return NULL;
			}
			lUVDiffuseElement->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
			lUVDiffuseElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

			FbxVector2 lVectors0(0, 0);
			FbxVector2 lVectors1(1, 0);
			FbxVector2 lVectors2(1, 1);
			FbxVector2 lVectors3(0, 1);

			lUVDiffuseElement->GetDirectArray().Add(lVectors0);
			lUVDiffuseElement->GetDirectArray().Add(lVectors1);
			lUVDiffuseElement->GetDirectArray().Add(lVectors2);
			lUVDiffuseElement->GetDirectArray().Add(lVectors3);


			// Create UV for Ambient channel
			FbxGeometryElementUV* lUVAmbientElement = lMesh->CreateElementUV("AmbientUV");

			lUVAmbientElement->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
			lUVAmbientElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

			lVectors0.Set(0, 0);
			lVectors1.Set(1, 0);
			lVectors2.Set(0, 0.418586879968643);
			lVectors3.Set(1, 0.418586879968643);

			lUVAmbientElement->GetDirectArray().Add(lVectors0);
			lUVAmbientElement->GetDirectArray().Add(lVectors1);
			lUVAmbientElement->GetDirectArray().Add(lVectors2);
			lUVAmbientElement->GetDirectArray().Add(lVectors3);

			// Create UV for Emissive channel
			FbxGeometryElementUV* lUVEmissiveElement = lMesh->CreateElementUV("EmissiveUV");

			lUVEmissiveElement->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
			lUVEmissiveElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

			lVectors0.Set(0.2343, 0);
			lVectors1.Set(1, 0.555);
			lVectors2.Set(0.333, 0.999);
			lVectors3.Set(0.555, 0.666);

			lUVEmissiveElement->GetDirectArray().Add(lVectors0);
			lUVEmissiveElement->GetDirectArray().Add(lVectors1);
			lUVEmissiveElement->GetDirectArray().Add(lVectors2);
			lUVEmissiveElement->GetDirectArray().Add(lVectors3);

			//Now we have set the UVs as eINDEX_TO_DIRECT reference and in eBY_POLYGON_VERTEX  mapping mode
			//we must update the size of the index array.
			lUVDiffuseElement->GetIndexArray().SetCount(24);
			lUVAmbientElement->GetIndexArray().SetCount(24);
			lUVEmissiveElement->GetIndexArray().SetCount(24);



			// Create polygons. Assign texture and texture UV indices.
			for (i = 0; i < 6; i++)
			{
				//we won't use the default way of assigning textures, as we have
				//textures on more than just the default (diffuse) channel.
				lMesh->BeginPolygon(-1, -1, false);



				for (j = 0; j < 4; j++)
				{
					//this function points 
					lMesh->AddPolygon(lPolygonVertices[i * 4 + j] // Control point index. 
					);
					//Now we have to update the index array of the UVs for diffuse, ambient and emissive
					lUVDiffuseElement->GetIndexArray().SetAt(i * 4 + j, j);
					lUVAmbientElement->GetIndexArray().SetAt(i * 4 + j, j);
					lUVEmissiveElement->GetIndexArray().SetAt(i * 4 + j, j);

				}

				lMesh->EndPolygon();
			}

			FbxNode* lNode = FbxNode::Create(mScene, "Cube_Node");

			lNode->SetNodeAttribute(lMesh);
			lNode->SetShadingMode(FbxNode::eTextureShading);

			CreateCubeTexture(mScene, lMesh);
			printf("Sending Cube \n");
			return lNode;
		}
	}
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

	FbxString meshstring = "Cube";
	// create scene for our created mesh
	FbxScene* lSceneCreated = CreateScene();
	// create texture
	CreateTexture(lSceneCreated);
	// create model from inputs 
	FbxNode* lMeshNodeCreated = CreateMesh(meshstring, lSceneCreated);
	// create an importer
	FbxImporter* lImporter = FbxImporter::Create(lManager, "");

	// use first argument as file name for importer
	// TODO: shouldn't hard code this
	if (!lImporter->Initialize("C:\\Users\\thoma\\modelcreator\\ModelCreator\\Debug\\terrain.fbx", -1, lManager->GetIOSettings()))
	{
		printf("Failure when initializing importer \n");
		printf("Error returned: %s\n\n", lImporter->GetStatus().GetErrorString());
		exit(-1);
	}

	FbxScene* lScene = FbxScene::Create(lManager, "My Scene");

	lImporter->Import(lScene); 

	lImporter->Destroy();

	// Apply texture to mesh
	//CreateTexture(lScene);

	//CreateMaterial(lScene);

	/* Grab terrain mesh from scene
	FbxMesh* lMesh = lScene->GetRootNode()->GetChild(0)->GetMesh();

	FbxNode* terrainNode = lScene->GetRootNode()->GetChild(0);

	int count = lScene->GetRootNode()->GetChildCount();
	*/
	/*
	if (lMesh->GetPolygonCount() > 0)
	{
		printf("Node visible, polygon count of %i \n", lMesh->GetPolygonCount());
	}
	else {
		printf("Node not visible \n");
	} */
	

	// AddMaterials(lMesh);

	// Export Scene

	bool r;

	r = SaveScene(lManager, lSceneCreated, "C:\\Users\\thoma\\modelcreator\\ModelCreator\\Debug\\pyramidtestwhite.fbx", -1, false);
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

