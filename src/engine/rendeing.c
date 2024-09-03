#include "rendeing.h"
#include "dataTypes.h"
#include "engine.h"
#include "engine_internal.h"
#include "errors.h"
#include "stringTools.h"
#include "window.h"
#include "stringStream.h"
#include "rendeing_math.h"
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

GLuint compileShader(char *ss, GLenum shaderType)
{
        GLuint shader = glCreateShader(shaderType);
        const char *shaderSource = ss;
        glShaderSource(shader, 1, &shaderSource, NULL);
        glCompileShader(shader);

        // Check for compilation errors
        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
                printf("\033[1;31m ERROR IN SHADER\033[0m");
        }

        free((void*)shaderSource);

        return shader;
}

GLuint buildShader(char *path, GLenum shaderType)
{
        char *type;
        switch (shaderType) {
                case GL_VERTEX_SHADER: 
                        type = "vertexShaders";
                        break;
                case GL_FRAGMENT_SHADER:
                        type = "fragmentShaders";
                        break;
        }
        char *ss = readShader(path, type);
        return compileShader(ss, shaderType);
}

GLuint attachShader(GLuint vertexShader, GLuint fragmentShader)
{
        GLuint shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);

        glLinkProgram(shaderProgram);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return shaderProgram;
}

void parce_vert_line(char *line, Object *obj)
{ 
        int d = 0;
        char **vertices = splitString(&line[3], ' ', &d);

        vec4 *out = &obj->vertices[obj->count];

        switch (line[1]) {

                case '4':
                        out->w = stringToFloat(vertices[3]);
                case '3':
                        out->z = stringToFloat(vertices[2]);
                case '2':
                        out->y = stringToFloat(vertices[1]);
                        out->x = stringToFloat(vertices[0]); 
        }
        obj->count++;
}

void parce_indices_line(char *line, Object *obj)
{ 
        int d = 0;
        char **indices = splitString(&line[2], ' ', &d);
        if (obj->indices != NULL) {
                FATAL_ERROR("There can only be one indices line in the manafest\n", incorrect_input);
        }
        obj->indices = allocate(sizeof(u8) * d);
        for (int i = 0; i < d; i++) {
                obj->indices[i] = stringToUChar(indices[i]);
        }
        obj->index_count = d;
}

void parce_line(char *line, Object *obj)
{
        if (line[0] == '\n') {
                return;
        }
        if (line[1] == 's') {
                if (line[0] == 'v') {
                        obj->shader.shaders.vertexShader = buildShader(&line[3], GL_VERTEX_SHADER);
                }
                if (line[0] == 'f') {
                        obj->shader.shaders.fragmentShader = buildShader(&line[3], GL_FRAGMENT_SHADER);
                }
                return;
        }
        if (line[0] == 'v') {
                parce_vert_line(line, obj);
                return;
        }
        if (line[0] == 'i') {
                parce_indices_line(line, obj);
        }

}

size_t vert_count(char **man_file, int line_count)
{
      int v_count = 0;
      for (int i = 0; i < line_count; i++) {
              char *line = man_file[i];
              if (line[0] == 'v'){
                      v_count++;
              }
      }
      return v_count;
}

Object *parce_manafest(char *name)
{
        Object *obj = allocate(sizeof(Object));
        char *contence = readRes(name);
        int substrings = 0;
        char **lines = splitString(contence, '\n', &substrings); 
        size_t v_count = vert_count(lines, substrings);
        obj->vertices = allocate(sizeof(vec4) * v_count);
        for (int i = 0; i < substrings; i++) {
                parce_line(lines[i], obj);
        }

        obj->shader.shaderProgram = attachShader(obj->shader.shaders.vertexShader, obj->shader.shaders.fragmentShader);
        GLuint transformLoc = glGetUniformLocation(obj->shader.shaderProgram, "transform");
        obj->transformMat = transformLoc;
        
        GLuint scale = glGetUniformLocation(obj->shader.shaderProgram, "scale");
        obj->scaleMat = scale;
        return  obj;
}

void print_v_c(Object *obj)
{
        for (size_t i = 0; i < obj->count; i++) {
                printf("Vertex %zu: (%f, %f)\n", i, obj->vertices[i].x, obj->vertices[i].y);
        }
}

//Object *genrate_object(char *vertexShader, char *fragmentShader, vec2 *vertices)
//{

//}

void pass_rotation_matrix(vec2 angle, Object *obj)
{
        mat4 a;
        rotation_matrix_x(angle, &a);
        glUniformMatrix4fv(obj->transformMat, 1, GL_FALSE, (const GLfloat*)a);
}

void scale_object(float scale, Object *obj)
{
        mat4 a;
        scale_matrix(scale, &a);
        glUniformMatrix4fv(obj->scaleMat, 1, GL_FALSE, (const GLfloat*)a);
}

void draw_triangle_mesh_GL(Object *obj, WindowState *window)
{
        float *vertices = allocate(sizeof(float) * 3 * obj->count);
        for (size_t i = 0;i < obj->count; i++) {
                vertices[i*3] = obj->vertices[i].x;
                vertices[i*3 + 1] = obj->vertices[i].y;
                vertices[i*3 + 2] = obj->vertices[i].z;
        }

        GLuint VAO;
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        GLuint VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER, obj->count * 3 * sizeof(float), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);
        
        glUseProgram(obj->shader.shaderProgram);
        
        vec2 angle = {window->totalTime, 2};
        pass_rotation_matrix(angle, obj);
        scale_object(0.5, obj);

        glBindVertexArray(VAO);

        if (obj->indices != NULL) {
                GLuint IBO;
                glGenBuffers(1, &IBO);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj->index_count * sizeof(u8), obj->indices, GL_STATIC_DRAW);
                glDrawElements(GL_TRIANGLES, obj->index_count, GL_UNSIGNED_BYTE, (GLvoid*)0);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
                glDeleteBuffers(1, &IBO);
        } else {
                glDrawArrays(GL_TRIANGLES, 0, obj->count);
        }
        glBindVertexArray(0);

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);

        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
                printf("OpenGL Error: %d\n", err);
        }
        deallocate(vertices);
}

void begin_frame()
{
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.5f, 1.0f, 0.0f, 1.0f);
}

void startRenderer(WindowState *window)
{
        printf("%f\n", window->deltaTime);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
}
