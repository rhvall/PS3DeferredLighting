//
//  Renderer.cpp
//  WhatTheFlock
//
//  Created by Raúl Valencia on 14/03/2012.
//  2012 Team Project.
//
//  Class Description: Render class that is called from
//	main to execute all commands that are required to
//  display the objects on the screen
//
//	           (\___/)
//             (o\ /o)
//            /|:.V.:|\
//		      \\::::://
//        -----`"" ""`-----

#include "Renderer.h"

Renderer::Renderer(void)
{
	/*
	You're provided with a very basic vertex / fragment shader, to get you started
	with Cg, and drawing textured objects. 
	*/
	rotation = 0.0f;

	lightVert = new VertexShader(LIGHT_VERTEX_SHADER);
	lightFrag = new FragmentShader(LIGHT_FRAGMENT_SHADER);

	basicVert = new VertexShader(BASIC_VERTEX_SHADER);
	basicFrag = new FragmentShader(BASIC_FRAGMENT_SHADER);

	skyboxVert = new VertexShader(SKYBOX_VERTEX_SHADER);
	skyboxFrag = new FragmentShader(SKYBOX_FRAGMENT_SHADER);

	sceneVert = new VertexShader(SCENE_VERTEX_SHADER);
	sceneFrag = new FragmentShader(SCENE_FRAGMENT_SHADER);
	
	pointLightVert = new VertexShader(POINT_VERTEX_SHADER);
	pointLightFrag = new FragmentShader(POINT_FRAGMENT_SHADER);
	
	combineVert = new VertexShader(COMBINE_VERTEX_SHADER);
	combineFrag = new FragmentShader(COMBINE_FRAGMENT_SHADER);
	
	texturedVert = new VertexShader(TEXTURED_VERTEX_SHADER);
	texturedFrag = new FragmentShader(TEXTURED_FRAGMENT_SHADER);

	this->SetCurrentShader(*basicVert,*basicFrag);

//	light = new Light(Vector3(300.0f, 300.0f, 300.0f),
//					  Vector4(0.9f, 0.9f, 0.9f, 0.8f),
//					  50000.0f);

    light = new Light[LIGHTS * LIGHTS];
    for(int x = 0; x < LIGHTS; ++x)
    {
        for(int z = 0; z < LIGHTS; ++z)
        {
            Light &l = light[(x * LIGHTS) + z];
            float xPos = (RAW_WIDTH * HEIGHTMAP_X / ( LIGHTS - 1)) * x;
            float zPos = (RAW_HEIGHT * HEIGHTMAP_Z / ( LIGHTS - 1)) * z;
            l.position = Vector3(xPos, 200.0f, zPos);

            float r = 0.5f + (float)(rand()%129) / 128.0f;
            float g = 0.5f + (float)(rand()%129) / 128.0f;
            float b = 0.5f + (float)(rand()%129) / 128.0f;
            l.colour = Vector4(r, g, b, 1.0f);
            l.radius = (RAW_WIDTH * HEIGHTMAP_X / LIGHTS);
        }
    }


	/*
	Projection matrix...0.7853982 is 45 degrees in radians.
	*/
	projMatrix = Matrix4::perspective(0.7853982, screen_ratio, 1.0f, 10000.0f);	//CHANGED TO THIS!!

	root = new SceneNode();
	root->SetTransform(Matrix4::identity());

	initMesh = Mesh::GenerateQuad();
	CellGcmTexture *initScreen = GCMRenderer::LoadTGA(INIT_SCREEN, false);
	initMesh->SetDefaultTexture(*initScreen);

	sphere = new OBJMesh(SPHERE_OBJ);
	
	cellGcmSetDepthTestEnable(CELL_GCM_TRUE);
	cellGcmSetCullFaceEnable(CELL_GCM_TRUE);
	cellGcmSetBlendEnable(CELL_GCM_TRUE);
	cellGcmSetDepthFunc(CELL_GCM_LESS);

	
	cellGcmSetDitherEnable(CELL_GCM_TRUE);
	//	std::cout << "cellGcmCgGetRegisterCount: " << cellGcmCgGetRegisterCount(sceneFrag->program) << std::endl;
	//cellGcmCgSetRegisterCount(sceneFrag->program, 20);
}

Renderer::~Renderer(void)
{
	delete basicVert;
	delete basicFrag;
	delete skyboxVert;
	delete skyboxFrag;
	delete lightVert;
	delete lightFrag;
	
	delete sceneVert;
	delete sceneFrag;
	delete pointLightVert;
	delete pointLightFrag;
	delete combineVert;
	delete combineFrag;
	
	delete []light;
	delete root;
	delete sphere;

	delete initMesh;
}

/*
Main rendering function. Note how it's essentially the same as the
ones you were writing in OpenGL! We start by clearing the buffer,
render some stuff, then swap the buffers. All that's different is
some slightly different matrix access.

*/
void Renderer::RenderScene()
{
	set_viewport();
	clear_buffer();

	drawSkybox();

	//fillBuffers();
	//drawPointLights();
	//combineBuffers();
	
	simpleLighting();
	
	swap_buffers();

	//std::cout << "Not crashed!!" << std::endl;
}

void Renderer::drawSkybox()
{
	//cellGcmSetSurface(&deferredLight[0]);
	//cellGcmSetSurface(&surfaces[swapValue]);
	clear_buffer();
	this->SetCurrentShader(*skyboxVert, *skyboxFrag);
	
	cellGcmSetDepthTestEnable(CELL_GCM_FALSE);
	
	modelMatrix = Matrix4::identity();
	viewMatrix = camera->BuildViewMatrix();
	Matrix4 aProjMatrix = Matrix4::orthographic(-0.5,0.5,-0.5,0.5,-1,1);
	currentVert->UpdateShaderMatrices(modelMatrix, viewMatrix, aProjMatrix);
	
	SetTextureSampler(currentFrag->GetParameter("texture"), root->GetMesh()->GetDefaultTexture());
	root->GetMesh()->Draw(*skyboxVert, *skyboxFrag);

	//CellGcmSurface aSuf = deferredLight[0];
	//std::cout << "Surf type: " << aSuf.colorOffset[0] << std::endl;
}

void Renderer::simpleLighting()
{
	this->SetCurrentShader(*lightVert, *lightFrag);
	//this->SetCurrentShader(*sceneVert, *sceneFrag);
	cellGcmSetDepthTestEnable(CELL_GCM_TRUE);
	cellGcmSetCullFaceEnable(CELL_GCM_TRUE);
	cellGcmSetBlendEnable(CELL_GCM_TRUE);
	cellGcmSetDepthFunc(CELL_GCM_LESS);

	viewMatrix = camera->BuildViewMatrix();
	currentVert->UpdateShaderMatrices(modelMatrix, viewMatrix, projMatrix);

	Vector3 translate = Vector3((RAW_HEIGHT * HEIGHTMAP_X / 2.0f),
								500,
								(RAW_HEIGHT * HEIGHTMAP_Z / 2.0f));
	Matrix4 pushMatrix = Matrix4::translation(translate);
	Matrix4 popMatrix = Matrix4::translation(-translate);
	modelMatrix = pushMatrix * 
				  Matrix4::rotation(rotation, Vector3(0, 1, 0)) *
				  popMatrix *
				  Matrix4::translation(light->position) * 
				  Matrix4::scale(Vector3(light->radius, light->radius, light->radius));

	light->position.setX(modelMatrix.getElem(3,0));
	light->position.setY(modelMatrix.getElem(3,1));
	light->position.setZ(modelMatrix.getElem(3,2));

	float *lPosition = (float *)&light->position;
	float *lColour = (float *)&light->colour;
	Vector3 cPosV = camera->GetPosition();
	float *cPos = (float *)&cPosV;
		
	currentFrag->SetParameter("cameraPos", cPos, false);
	setLightShaderUniform(lPosition, lColour, &light->radius);

	DrawNode(root);
}

void Renderer::renderInitScreen()
{
	this->SetCurrentShader(*basicVert, *basicFrag);
	
	Matrix4 aProjMatrix = Matrix4::orthographic(-1, 1, -1, 1, 0, 1);
	viewMatrix = Matrix4::identity();
	modelMatrix = Matrix4::identity();
	
	set_viewport();
	clear_buffer();

	cellGcmSetDepthTestEnable(CELL_GCM_FALSE);
	//cellGcmSetDepthFunc(CELL_GCM_LESS);

	currentVert->UpdateShaderMatrices(modelMatrix, viewMatrix, aProjMatrix);
	SetTextureSampler(currentFrag->GetParameter("texture"), initMesh->GetDefaultTexture());
	initMesh->Draw(*basicVert, *basicFrag);
	swap_buffers();
}

void Renderer::fillBuffers()
{
	cellGcmSetSurface(&deferredLight[0]);
	cellGcmSetDitherEnable(CELL_GCM_FALSE);
	clear_buffer();
	this->SetCurrentShader(*sceneVert, *sceneFrag);
	cellGcmSetDepthTestEnable(CELL_GCM_TRUE);
	cellGcmSetCullFaceEnable(CELL_GCM_TRUE);
	cellGcmSetBlendEnable(CELL_GCM_TRUE);
	cellGcmSetDepthFunc(CELL_GCM_LESS);

	cellGcmSetColorMaskMrt(CELL_GCM_COLOR_MASK_MRT1_A |
						   CELL_GCM_COLOR_MASK_MRT1_R |
						   CELL_GCM_COLOR_MASK_MRT1_G |
						   CELL_GCM_COLOR_MASK_MRT1_B);



	projMatrix = Matrix4::perspective(0.7853982, screen_ratio, 1.0f, 10000.0f);	//CHANGED TO THIS!!
	//projMatrix = Matrix4::orthographic(-0.5,0.5,-0.5,0.5,-1,1);
	viewMatrix = camera->BuildViewMatrix();
	modelMatrix = Matrix4::identity();
	currentVert->UpdateShaderMatrices(modelMatrix, viewMatrix, projMatrix);

	DrawNode(root);
	cellGcmSetDitherEnable(CELL_GCM_TRUE);

	cellGcmSetColorMaskMrt(0);
	//SetTextureSampler(currentFrag->GetParameter("texture"), height->GetMesh()->GetDefaultTexture());
	//height->GetMesh()->Draw(*sceneVert, *sceneFrag);
}

void Renderer::drawPointLights()
{
	cellGcmSetSurface(&deferredLight[1]);
	cellGcmSetDitherEnable(CELL_GCM_FALSE);
	clear_buffer();
	this->SetCurrentShader(*pointLightVert, *pointLightFrag);
	
	cellGcmSetColorMaskMrt(CELL_GCM_COLOR_MASK_MRT1_A |
						   CELL_GCM_COLOR_MASK_MRT1_R |
						   CELL_GCM_COLOR_MASK_MRT1_G |
						   CELL_GCM_COLOR_MASK_MRT1_B);

	//cellGcmSetClearColor((0<<0)|(0<<8)|(0<<16)|(255<<24)); //glClearColor(0,0,0,1);
	//cellGcmSetClearSurface(CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B);
	cellGcmSetBlendEnable(CELL_GCM_TRUE);
	cellGcmSetBlendFunc(CELL_GCM_ONE, CELL_GCM_ONE, CELL_GCM_ONE, CELL_GCM_ONE);
	
	textureFromSurface(depthTexture, deferredLight[0], -1);
	textureFromSurface(normalTexture, deferredLight[0], 1);

	SetTextureSampler(currentFrag->GetParameter("depthTex"), &depthTexture);
	SetTextureSampler(currentFrag->GetParameter("normTex"), &normalTexture);
		
	Vector3 cPosV = camera->GetPosition();
	float pixelSize[2] = {1.0f / screen_width, 1.0f / screen_height};
	currentFrag->SetParameter("cameraPos", (float *)&cPosV, false);
	currentFrag->SetParameter("pixelSize", pixelSize, false);

	Vector3 translate = Vector3((RAW_HEIGHT * HEIGHTMAP_X / 2.0f),
								500,
								(RAW_HEIGHT * HEIGHTMAP_Z / 2.0f));
	Matrix4 pushMatrix = Matrix4::translation(translate);
	Matrix4 popMatrix = Matrix4::translation(-translate);

	for(int x = 0; x < LIGHTS; ++x)
	{
		for(int z = 0; z < LIGHTS; ++z)
		{
			Light &l = light[(x * LIGHTS) + z];
			float radius = l.radius;

			modelMatrix = pushMatrix * 
						  Matrix4::rotation(rotation, Vector3(0, 1, 0)) *
						  popMatrix *
						  Matrix4::translation(l.position) * 
						  Matrix4::scale(Vector3(radius, radius, radius));

			l.position.setX(modelMatrix.getElem(3,0));
			l.position.setY(modelMatrix.getElem(3,1));
			l.position.setZ(modelMatrix.getElem(3,2));

			float *lPosition = (float *)&light->position;
			float *lColour = (float *)&light->colour;
			setLightShaderUniform(lPosition, lColour, &radius);
			currentVert->UpdateShaderMatrices(modelMatrix, viewMatrix, projMatrix);

			Vector3 val = l.position - cPosV;
			float dist = length(val);

			if(dist < radius)
			{
				cellGcmSetCullFace(CELL_GCM_FRONT);
			}
			else
			{
				cellGcmSetCullFace(CELL_GCM_BACK);
			}
			sphere->Draw(*pointLightVert, *pointLightFrag);
		}
	}
	
	//cellGcmSetCullFace(CELL_GCM_BACK);
	//cellGcmSetBlendFunc(CELL_GCM_ONE, CELL_GCM_ZERO, CELL_GCM_ONE, CELL_GCM_ZERO);
	//glBlendFunc ( GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA );
	//cellGcmSetClearColor((64<<0)|(64<<8)|(64<<16));

	cellGcmSetColorMaskMrt(0);
	//glClearColor (0.2f ,0.2f ,0.2f ,1);
	//glBindFramebuffer ( GL_FRAMEBUFFER , 0);
	//glUseProgram (0);
	
}

void Renderer::combineBuffers()
{
	cellGcmSetSurface(&surfaces[swapValue]);
	cellGcmSetDitherEnable(CELL_GCM_TRUE);
	clear_buffer();
	cellGcmSetCullFace(CELL_GCM_BACK);
	cellGcmSetBlendFunc(CELL_GCM_ONE, CELL_GCM_ZERO, CELL_GCM_ONE, CELL_GCM_ZERO);

	this->SetCurrentShader(*combineVert, *combineFrag);
	
	//cellGcmSetDitherEnable(CELL_GCM_TRUE);
	
	Matrix4 aProjMatrix = Matrix4::orthographic(-1, 1, -1, 1, 0, 1);
	viewMatrix = Matrix4::identity();
	modelMatrix = Matrix4::identity();
	//viewMatrix.setElem(1,1,-1.0f);
	//aProjMatrix.setElem(1,1,2.0f);
	modelMatrix = Matrix4::identity();
	currentVert->UpdateShaderMatrices(modelMatrix, viewMatrix, aProjMatrix);

	//textureFromSurface(lightEmissiveTexture, deferredLight[1]);
	//textureFromSurface(lightSpecularTexture, deferredLight[1], 1);	
	
	//textureFromSurface(colorTexture, deferredLight[0], 0);
	//textureFromSurface(colorTexture, deferredLight[0], 0);
	textureFromSurface(colorTexture, deferredLight[1], 0);
	//textureFromSurface(normalTexture, deferredLight[0], 1);

	SetTextureSampler(currentFrag->GetParameter("diffuseTex"), &colorTexture);
	//SetTextureSampler(currentFrag->GetParameter("emissiveTex"), &colorTexture);
	//SetTextureSampler(currentFrag->GetParameter("specularTex"), &colorTexture);
	//SetTextureSampler(currentFrag->GetParameter("emissiveTex"), &lightEmissiveTexture);
	//SetTextureSampler(currentFrag->GetParameter("specularTex"), &lightSpecularTexture);
	
	currentFrag->UpdateShaderVariables();
	
	initMesh->Draw(*combineVert, *combineFrag);	
}

void Renderer::drawToBuffer()
{
	cellGcmSetSurface(&surfaces[swapValue]);
	clear_buffer();

	this->SetCurrentShader(*texturedVert, *texturedFrag);
	//this->SetCurrentShader(*basicVert, *basicFrag);
	Matrix4 aProjMatrix = Matrix4::orthographic(-1, 1, -1, 1, 0, 1);
	viewMatrix = Matrix4::identity();
	viewMatrix.setElem(1,1,-1.0f);
	modelMatrix = Matrix4::identity();
	cellGcmSetDepthTestEnable(CELL_GCM_FALSE);
	currentVert->UpdateShaderMatrices(modelMatrix, viewMatrix, aProjMatrix);
	
	textureFromSurface(colorTexture, deferredLight[0]);
	initMesh->SetDefaultTexture(colorTexture);
	SetTextureSampler(currentFrag->GetParameter("texture"), initMesh->GetDefaultTexture());
	initMesh->Draw(*texturedVert, *texturedFrag);
	//initMesh->Draw(*basicVert, *basicFrag);
}

void Renderer::textureFromSurface(CellGcmTexture &texture, const CellGcmSurface &gcmSurface, const int &colourBuffer)
{
	//uint32_t format;
	//uint32_t remap;

	////cellGcmUtilGetTextureAttribute(CELL_GCM_UTIL_ARGB8,
	////									   &format, &remap,
	////									   mEnableSwizzle ? 1 : 0, 1);
	//cellGcmUtilGetTextureAttribute(CELL_GCM_UTIL_ARGB8,
	//							   &format, &remap,
	//							   0, 1);

	texture.width  = screen_width;
	texture.height = screen_height;
	texture.pitch  = (colourBuffer < 0) ? gcmSurface.depthPitch : gcmSurface.colorPitch[colourBuffer];
	texture.depth  = 1;
	texture.location = CELL_GCM_LOCATION_LOCAL;	//We want the texture in graphics memory
	texture.offset = (colourBuffer < 0) ? gcmSurface.depthOffset : gcmSurface.colorOffset[colourBuffer];						//at the following offset
	//texture.format = (colourBuffer < 0) ? CELL_GCM_TEXTURE_DEPTH24_D8 : CELL_GCM_TEXTURE_A8R8G8B8;	//ARGB format
	texture.format = CELL_GCM_TEXTURE_A8R8G8B8;	//ARGB format
	texture.format |= CELL_GCM_TEXTURE_LN;		//Data is a 'linear' array of values
	//texture.format |= CELL_GCM_TEXTURE_NR;
	texture.remap  = 
		//CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
		//CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
		//CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
		//CELL_GCM_TEXTURE_REMAP_REMAP <<  8 |
		//CELL_GCM_TEXTURE_REMAP_FROM_A << 6 |
		//CELL_GCM_TEXTURE_REMAP_FROM_R << 4 |
		//CELL_GCM_TEXTURE_REMAP_FROM_G << 2 |
		//CELL_GCM_TEXTURE_REMAP_FROM_B;
		CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
		CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
		CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
		CELL_GCM_TEXTURE_REMAP_REMAP << 8 |
		CELL_GCM_TEXTURE_REMAP_FROM_B << 6 |
		CELL_GCM_TEXTURE_REMAP_FROM_G << 4 |
		CELL_GCM_TEXTURE_REMAP_FROM_R << 2 |
		CELL_GCM_TEXTURE_REMAP_FROM_A;

	texture.mipmap		= 1;		//How many mipmap levels are there (1 == 'largest level mipmap')
	texture.cubemap	= CELL_GCM_FALSE;	//No...it's not a cubemap
	texture.dimension = CELL_GCM_TEXTURE_DIMENSION_2;	//TODO: This will presumably mean power of 2 sizes only!
}
