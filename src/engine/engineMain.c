#include "dataTypes.h"
#include "engine.h"
#include "window.h"
#include "rendeing.h"
#include <stdio.h>

void handleEvent(WindowState *windowState) 
{        
        SDL_Event event;
        
        if (SDL_PollEvent(&event)) {
                switch (event.type) {
                case SDL_QUIT:
                        windowState->running = 0;
                        break;
                }
        }
}

void mainLoop(WindowState *windowState)
{
        f32 acumulator = 0;
        f32 target = 1;
        f32 dt = 0;
        u32 preTicks = SDL_GetTicks();
        u32 curTicks = 0;

        startRenderer();

        Object *obj = parce_manafest("basic.man");

        while (windowState->running) {
                curTicks = SDL_GetTicks();
                windowState->deltaTime = (curTicks - preTicks)/ 1000.0f;
                windowState->totalTime += windowState->deltaTime;
                preTicks = curTicks;
                dt = windowState->deltaTime;
                acumulator += dt;
                if (acumulator >= target) {

                        acumulator -= target;
                }

                begin_frame();

                draw_triangle_mesh_GL(obj, windowState);
                SDL_GL_SwapWindow(windowState->window);

                handleEvent(windowState);
                
                GLenum error = glGetError();
                if(error != GL_NO_ERROR) {
                        printf("OpenGL error: %d\n", error);
                }
        }
        printf("main loop exited at %f second elapsed\n", windowState->totalTime);
}

void engineMain()
{
        WindowState *window = WindowInit();
        mainLoop(window);
}
