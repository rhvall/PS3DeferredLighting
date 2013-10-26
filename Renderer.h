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

#pragma once
#include <cell/gcm.h>
#include "Graphics/GCMRenderer.h"
#include "Mesh/Mesh.h"
#include "Mesh/OBJMesh.h"
#include "Graphics/Light.h"

#define LIGHTS 4

class Renderer : public GCMRenderer
{
public:
	Renderer(void);
	~Renderer(void);

	virtual void RenderScene();
	void renderInitScreen();
	
	inline Mesh *getInitMesh(){return initMesh;}

	float rotation;
	
	SceneNode *height;

protected:
	void drawSkybox();
	void simpleLighting();
	void fillBuffers();
	void drawPointLights();
	void combineBuffers();
	void drawToBuffer();
	void textureFromSurface(CellGcmTexture &texture, const CellGcmSurface &gcmSurface, const int &colourBuffer = 0);

	VertexShader *basicVert;
	FragmentShader *basicFrag;
	VertexShader *lightVert;
	FragmentShader *lightFrag;
	VertexShader *skyboxVert;
	FragmentShader *skyboxFrag;

	VertexShader *sceneVert;
	FragmentShader *sceneFrag;
	VertexShader *pointLightVert;
	FragmentShader *pointLightFrag;
	VertexShader *combineVert;
	FragmentShader *combineFrag;
	VertexShader *texturedVert;
	FragmentShader *texturedFrag;

	Light *light;

	Mesh *initMesh;
	OBJMesh *sphere;

	CellGcmTexture colorTexture;
	CellGcmTexture normalTexture;
	CellGcmTexture depthTexture;
	CellGcmTexture lightEmissiveTexture;
	CellGcmTexture lightSpecularTexture;
};
