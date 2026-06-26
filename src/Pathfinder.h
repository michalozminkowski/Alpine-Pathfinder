#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "Snow.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

class Pathfinder {
public:
  Pathfinder(SnowSimulation *sim);
  ~Pathfinder();

  void initRendering(GLuint shaderProgram);

  void generatePath(int cornerIdx, float snowLevel);

  void clearPaths();

  void render(const glm::mat4 &view, const glm::mat4 &projection);

private:
  struct PathData {
    std::vector<glm::vec3> vertices;
    glm::vec4 color;
    int vertexOffset;
    int vertexCount;
    float distanceKm;
    float elevationGainM;
  };

  SnowSimulation *sim;
  GLuint vao, vbo;
  std::vector<PathData> paths;
  GLuint shader;

public:
  const std::vector<PathData> &getPaths() const { return paths; }

private:
  int findSummitIndex();
  std::vector<glm::vec3> findPathAStar(int startIdx, int targetIdx,
                                       float snowLevel);
  int getValidCorner(int cornerIdx);

  void updateVBO();
};

#endif
