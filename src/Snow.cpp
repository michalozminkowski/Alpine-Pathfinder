#include "Snow.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <glm/gtc/matrix_transform.hpp>

SnowSimulation::SnowSimulation(int numParticles) {
}

void SnowSimulation::initParticles(int numParticles) {
  particles.resize(numParticles);
  renderData.resize(numParticles);
  activeParticleCount = numParticles;

  // Initial spawn
  for (auto &p : particles) {
    respawn(p);
    p.life = ((float)rand() / RAND_MAX) * 10.0f;        // Random initial life
    p.position.y -= ((float)rand() / RAND_MAX) * 10.0f; // distribute initially
  }
}

SnowSimulation::~SnowSimulation() {
  if (vao != 0)
    glDeleteVertexArrays(1, &vao);
  if (vbo != 0)
    glDeleteBuffers(1, &vbo);
}

void SnowSimulation::respawn(Snowflake &p) {
  float x = minX + ((float)rand() / RAND_MAX) * (maxX - minX);
  float z = minZ + ((float)rand() / RAND_MAX) * (maxZ - minZ);
  float y = 15.0f + ((float)rand() / RAND_MAX) * 15.0f; // spawn much higher above mountain

  p.position = glm::vec3(x, y, z);
  p.velocity = glm::vec3(0, 0, 0);
  p.life = 10.0f + ((float)rand() / RAND_MAX) * 5.0f;
  p.size = 1.0f + ((float)rand() / RAND_MAX) * 2.0f;
}

bool SnowSimulation::barycentric(glm::vec2 p, glm::vec2 a, glm::vec2 b,
                                 glm::vec2 c, float &u, float &v, float &w) {
  glm::vec2 v0 = b - a, v1 = c - a, v2 = p - a;
  float d00 = glm::dot(v0, v0);
  float d01 = glm::dot(v0, v1);
  float d11 = glm::dot(v1, v1);
  float d20 = glm::dot(v2, v0);
  float d21 = glm::dot(v2, v1);
  float denom = d00 * d11 - d01 * d01;
  if (denom == 0.0f)
    return false;
  v = (d11 * d20 - d01 * d21) / denom;
  w = (d00 * d21 - d01 * d20) / denom;
  u = 1.0f - v - w;
  return (v >= 0.0f && w >= 0.0f && (v + w) <= 1.0f);
}

void SnowSimulation::initGridFromModel(const obj::Model &model,
                                       const glm::mat4 &modelMatrix) {
  std::cout << "Initializing CPU Snow 2D Height Grid..." << std::endl;

  // Find bounds
  minX = minZ = 99999.0f;
  maxX = maxZ = -99999.0f;

  std::vector<glm::vec3> worldVertices;
  worldVertices.reserve(model.vertex.size() / 3);
  for (size_t i = 0; i < model.vertex.size(); i += 3) {
    glm::vec4 pos(model.vertex[i], model.vertex[i + 1], model.vertex[i + 2],
                  1.0f);
    pos = modelMatrix * pos;
    worldVertices.push_back(glm::vec3(pos));

    if (pos.x < minX)
      minX = pos.x;
    if (pos.x > maxX)
      maxX = pos.x;
    if (pos.z < minZ)
      minZ = pos.z;
    if (pos.z > maxZ)
      maxZ = pos.z;
  }

  // Add margin
  minX -= 1.0f;
  maxX += 1.0f;
  minZ -= 1.0f;
  maxZ += 1.0f;

  cellSizeX = (maxX - minX) / gridWidth;
  cellSizeZ = (maxZ - minZ) / gridHeight;
  heightGrid.resize(gridWidth * gridHeight);

  // Process faces
  for (const auto &group : model.faces) {
    const auto &indices = group.second;
    for (size_t i = 0; i < indices.size(); i += 3) {
      int i0 = indices[i];
      int i1 = indices[i + 1];
      int i2 = indices[i + 2];

      glm::vec3 v0 = worldVertices[i0];
      glm::vec3 v1 = worldVertices[i1];
      glm::vec3 v2 = worldVertices[i2];

      // Triangle normal
      glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
      if (normal.y < 0)
        normal = -normal; // ensure upward facing

      // Find 2D bounding box
      float t_minX = std::min({v0.x, v1.x, v2.x});
      float t_maxX = std::max({v0.x, v1.x, v2.x});
      float t_minZ = std::min({v0.z, v1.z, v2.z});
      float t_maxZ = std::max({v0.z, v1.z, v2.z});

      int minGx = std::max(0, (int)((t_minX - minX) / cellSizeX));
      int maxGx = std::min(gridWidth - 1, (int)((t_maxX - minX) / cellSizeX));
      int minGz = std::max(0, (int)((t_minZ - minZ) / cellSizeZ));
      int maxGz = std::min(gridHeight - 1, (int)((t_maxZ - minZ) / cellSizeZ));

      for (int gz = minGz; gz <= maxGz; ++gz) {
        for (int gx = minGx; gx <= maxGx; ++gx) {
          float cx = minX + (gx + 0.5f) * cellSizeX;
          float cz = minZ + (gz + 0.5f) * cellSizeZ;

          float u, v, w;
          if (barycentric(glm::vec2(cx, cz), glm::vec2(v0.x, v0.z),
                          glm::vec2(v1.x, v1.z), glm::vec2(v2.x, v2.z), u, v,
                          w)) {
            float cy = u * v0.y + v * v1.y + w * v2.y;
            int idx = gz * gridWidth + gx;
            if (cy > heightGrid[idx].height) {
              heightGrid[idx].height = cy;
              heightGrid[idx].normal = normal;
            }
          }
        }
      }
    }
  }
  std::cout << "Height Grid Generated." << std::endl;
}

GridCell SnowSimulation::getMountainData(float x, float z) {
  int gx = (int)((x - minX) / cellSizeX);
  int gz = (int)((z - minZ) / cellSizeZ);
  if (gx < 0 || gx >= gridWidth || gz < 0 || gz >= gridHeight) {
    return GridCell(); // Default returns very low height
  }
  return heightGrid[gz * gridWidth + gx];
}

void SnowSimulation::initRendering(GLuint shaderProgram) {
  shader = shaderProgram;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, renderData.size() * sizeof(glm::vec3), nullptr,
               GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);

  glBindVertexArray(0);
}

void SnowSimulation::update(float dt, float time) {
  for (int i = 0; i < activeParticleCount; ++i) {
    auto &p = particles[i];

    GridCell cell = getMountainData(p.position.x, p.position.z);

    // If the particle is already on the ground, freeze it completely
    if (p.position.y <= cell.height + 0.05f) {
      p.position.y = cell.height + 0.02f;
      p.velocity = glm::vec3(0.0f);
      p.life -= dt * 2.0f; // Melt faster on ground
    } else {
      // Apply physics if in the air
      p.velocity.y -= gravity * dt;
      p.velocity.x += sin(time * 0.5f + p.position.y) * windStrength * dt;
      p.velocity.z += cos(time * 0.8f + p.position.x) * windStrength * dt;

      p.position += p.velocity * dt;
      p.life -= dt;

      // Check collision after moving
      if (p.position.y <= cell.height) {
        p.position.y = cell.height + 0.02f;
        p.velocity = glm::vec3(0.0f);
      }
    }

    if (p.life <= 0.0f || p.position.y < -25.0f) {
      respawn(p);
    }
  }
}

void SnowSimulation::render(const glm::mat4 &view,
                            const glm::mat4 &projection) {
  if (activeParticleCount <= 0)
    return;

  for (int i = 0; i < activeParticleCount; ++i) {
    renderData[i] = particles[i].position;
  }

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, activeParticleCount * sizeof(glm::vec3),
                  renderData.data());

  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glDepthMask(GL_FALSE);

  glUseProgram(shader);

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(0.0f, 14.0f, 0.0f));
  glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE,
                     &model[0][0]);
  glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE,
                     &view[0][0]);
  glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE,
                     &projection[0][0]);

  glBindVertexArray(vao);
  glDrawArrays(GL_POINTS, 0, activeParticleCount);
  glBindVertexArray(0);

  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
}
