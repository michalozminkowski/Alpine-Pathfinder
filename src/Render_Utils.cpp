#include "Render_Utils.h"
#include "objload.h"

#include <iostream>

void Core::drawVAO(GLuint VAO, int numVertices)
{
    glBindVertexArray(VAO);

    glDrawArrays(GL_TRIANGLES, 0, numVertices);

    glBindVertexArray(0);
}

void Core::drawVAOIndexed(GLuint VAO, int numVertices)
{
    glBindVertexArray(VAO);

    glDrawElements(GL_TRIANGLES, numVertices, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}


    
GLuint Core::initVAO(const float* vertexArray, int numVertices, int elementSize) {
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);


    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, numVertices * elementSize * sizeof(float), vertexArray, GL_STATIC_DRAW);

    glVertexAttribPointer(0, elementSize, GL_FLOAT, GL_FALSE, elementSize * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);
    return VAO;
}

GLuint Core::initVAOIndexed(const float* vertexArray, const unsigned int* indexArray, int numVertices, int elementSize, int numIndexes) {
    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);


    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, numVertices * elementSize * sizeof(float), vertexArray, GL_STATIC_DRAW);

    glVertexAttribPointer(0, elementSize, GL_FLOAT, GL_FALSE, elementSize * sizeof(float), (void*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndexes*sizeof(unsigned int), indexArray, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);
    return VAO;
}

GLuint Core::initVAOForModel(const obj::Model& model) {
    GLuint VAO, VBO_pos, VBO_normal, VBO_tex, EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Positions (Location 0)
    if (!model.vertex.empty()) {
        glGenBuffers(1, &VBO_pos);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_pos);
        glBufferData(GL_ARRAY_BUFFER, model.vertex.size() * sizeof(float), model.vertex.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    // Normals (Location 1)
    if (!model.normal.empty()) {
        glGenBuffers(1, &VBO_normal);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal);
        glBufferData(GL_ARRAY_BUFFER, model.normal.size() * sizeof(float), model.normal.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
    }

    // Texture Coordinates (Location 2)
    if (!model.texCoord.empty()) {
        glGenBuffers(1, &VBO_tex);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_tex);
        glBufferData(GL_ARRAY_BUFFER, model.texCoord.size() * sizeof(float), model.texCoord.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(2);
    }

    // Index buffer (EBO)
    if (!model.faces.empty()) {
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        const auto& indices = model.faces.begin()->second;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), indices.data(), GL_STATIC_DRAW);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return VAO;
}

void Core::drawVAOIndexedUShort(GLuint VAO, int numIndices) {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
}

void Core::deleteVAO(GLuint VAO) {
    glDeleteVertexArrays(1, &VAO);
}


