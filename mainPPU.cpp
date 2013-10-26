//
//  Main.cpp
//  WhatTheFlock
//
//  Created by Raúl Valencia on 14/03/2012.
//  2012 Team Project.
//
//  Class Description: Main render loop, as a good
//	C++ citizen, everything starts here
//
//	           (\___/)
//             (o\ /o)
//            /|:.V.:|\
//		      \\::::://
//        -----`"" ""`-----


#include <stdio.h>
#include <stdlib.h>	

#include <string>
#include <iostream>

#include <sys/process.h>
#include <vectormath/cpp/vectormath_aos.h>
#include <pthread.h>
#include <time.h>

#include "Periferals/Input.h"
#include "Physics/Physics.h"
#include "Physics/Timer.h"

#include "Renderer.h"
#include "Mesh/HeightMap.h"
#include "Mesh/OBJMesh.h"
#include "Sound/Sound.h"
#include "Network/NetworkSockets.h"

#include "InputButtonFunctions.h"

SYS_PROCESS_PARAM(1001, 0x10000)

Renderer renderer;
SceneNode *root;

/*
Here's a quick example program. It'll load up a single SceneNode
scene, containing a textured quad, with a camera controllable by
the joypad. Pretty basic, but as with Graphics for Games, its
everything you need to get started and doing something more 
interesting with the Playstation!
*/

int main(void)
{
	srand ((unsigned int)time(NULL));
	std::cout << "All-New intro to GCM V1.0!\n" << std::endl;

	renderer.renderInitScreen();
	renderer.renderInitScreen();
	
	unsigned short done = 0;
	unsigned short networkMessage;
	

	//Start off by initialising the Input system
	Input::Initialise();
	////If the start button is pressed, call this function!
#ifdef DEFERRED_LIGHTING
	Input::SetPadFunction(INPUT_START, startButton, static_cast<void *>(&done));
	//Input::SetPadFunction(INPUT_TRIANGLE, triangleButton, NULL);
	Input::SetPadFunction(INPUT_SELECT, selectButton, static_cast<void *>(&networkMessage));
#endif
	Timer gameTime;

	//Make a new quad mesh, and set its texture to a newcastle logo
	//Create a new scenenode

	SceneNode *root = renderer.getRootNode();

#ifdef DEFERRED_LIGHTING
	Physics *physx = new Physics(root);
#endif

	HeightMap *heightMap = new HeightMap(HEIGHTMAP_RAW);
	heightMap->setDefaultTexture(HEIGHTMAP_TEXTURE);
	heightMap->setBumpTexture(HEIGHTMAP_BUMPMAP);
	SceneNode *heightMapNode = new SceneNode(heightMap, Vector3(1), Vector3(1), HEIGHTMAP);
	heightMapNode->invInertia = Matrix4(0);
	heightMapNode->mass = 1000.0f;
	root->AddChild(*heightMapNode);
	renderer.height = heightMapNode;

#ifdef DEFERRED_LIGHTING
	OBJMesh *aSheep = new OBJMesh(SHEEP_OBJ);
	aSheep->setDefaultTexture(SHEEP_TEXTURE);
	aSheep->setBumpTexture(SHEEP_BUMPMAP);

	static unsigned short NUMBER_OF_SHEEP = 3;
	SceneNode *sheepsArray[NUMBER_OF_SHEEP];

	for(int i = 0; i < NUMBER_OF_SHEEP; i++)
	{
		SceneNode *sphereNode = new SceneNode(aSheep, HEIGHTMAP_MIDDLE + Vector3(i * 100.0f, 200.0f, i * 50.0f), 
								Vector3(100.0f, 100.0f, 100.0f), SPHERE);
		sphereNode->collisionable = true;
		sheepsArray[i] = sphereNode;
		root->AddChild(*sphereNode);
	}

	//Input::SetPadFunction(INPUT_SELECT, triangleButton, (void *)&);
	Input::SetPadFunction(INPUT_CROSS, crossButton, static_cast<void *>(&(sheepsArray[0]->force)));

	OBJMesh *aCube = new OBJMesh(CUBE_OBJ);
	//aCube->setDefaultTexture("/Resources/CubeTexture.tga");
	SceneNode *cubeNode = new SceneNode(aCube, HEIGHTMAP_MIDDLE,
										CUBE_SCALE, CUBE);
	cubeNode->invInertia = Matrix4(0);
	cubeNode->mass = 1000.0f;
	cubeNode->boundingRadius = 1;
	root->AddChild(*cubeNode);
#endif

	//////////////////// Sound ////////////////
#ifdef PRESENTATION	
	pthread_t soundThread = NULL;
	int retVal = 0;
	unsigned short playSampleSound = 1;
	retVal = std::pthread_create(&soundThread, NULL, &startSound, (void *)&playSampleSound);

	if(retVal)
	{
		std::cout << "Something wrong with sound..." << std::endl;
	}
	extern volatile bool s_receivedExitGameRequest;
	
	//playSampleSound = 1;
#endif
	//////////////////// Sound ////////////////

	//////////////////// Network ////////////////
#ifdef PRESENTATION
	initializeServer();
	networkMessage = 0;
	pthread_t networkThread;
	retVal = 0;

	retVal = std::pthread_create(&networkThread, NULL, &serverLoop, (void *)&networkMessage);
#endif
	if(retVal)
	{
		std::cout << "Something wrong with network..." << std::endl;
	}
	//////////////////// Network ////////////////
#ifdef DEFERRED_LIGHTING
	Mesh *quadMesh = Mesh::GenerateQuad();
	quadMesh->setDefaultTexture(CUBE_MAP);
	root->SetMesh(quadMesh);
#endif
	
	//We need a new camera!
	CameraNode *n = new CameraNode();
	n->SetYaw(90.0f);
	n->SetPitch(-20.0f);
	n->SetControllingPad(JOYPAD_A);	//Controlled via joypad A
	n->SetPosition(Vector3(0.0f, 600.0f, 1500.0f)); //And set back slightly so we can see the node at the origin
	renderer.SetCamera(n);	//Set the current renderer camera
	root->AddChild(*n);

#ifdef PRESENTATION
	Mesh *initMesh = renderer.getInitMesh();
	initMesh->setDefaultTexture(MAIN_SCREEN);
#endif
	
	while(done == 0)
	{
		while(networkMessage == 0)
		{
			Input::UpdateJoypad();
			renderer.renderInitScreen();			
		}

		if(networkMessage == 2)
		{
#ifdef PRESENTATION			
			CellGcmTexture *endScreen = GCMRenderer::LoadTGA(END_SCREEN, false);
			initMesh->setDefaultTexture(END_SCREEN);
#endif
			for(int i = 0; i < NUMBER_OF_SHEEP; i++)
			{
				SceneNode *sheep = sheepsArray[i];
				sheep->objType = NOOBJ;
				sheep->orientation = Quat(0.0f,0.0f, 1.0f, 0.0f);
				sheep->collisionable = false;
			}
			
			playSampleSound = 2;
			renderer.renderInitScreen();
			
			sys_timer_usleep(1000000);
			networkMessage++;
			playEndScene(gameTime, *root, renderer);

			for(int i = 0; i < NUMBER_OF_SHEEP; i++)
			{
				SceneNode *sheep = sheepsArray[i];
				sheep->objType = SPHERE;
				sheep->orientation = Quat(0.0f,0.0f, 0.0f, 1.0f);
				sheep->collisionable = true;
			}

			while(networkMessage == 3 && done == 0)
			{
				renderer.renderInitScreen();
			}
		}
		else if(networkMessage == 5)
		{
			done = true;
		}

		Input::UpdateJoypad();	//Receive latest joypad input for all joypads

		float msec = (float)gameTime.GetTimedMS();

		renderer.rotation = msec * 0.005f;

#ifdef DEFERRED_LIGHTING
		physx->update(msec);
#endif
		root->Update(msec);	//Update our scene hierarchy. This bit is new (previously the renderer handled it)
		renderer.RenderScene();	//Render the scene
	}
	//If we get here, joypad A has had its start button pressed

	std::cout << "Quitting..." << std::endl;
	networkMessage = 5;
	s_receivedExitGameRequest = true;
	pthread_join(soundThread, NULL);
	pthread_join(networkThread, NULL);
	terminateServer();
#ifdef DEFERRED_LIGHTING
	delete physx;
#endif

	//delete minusX;
	//delete plusY;
	//delete minusY;
	//delete plusZ;
	//delete minusZ;

	Input::Destroy();

	std::cout << "Finished" << std::endl;
	return 0;
}