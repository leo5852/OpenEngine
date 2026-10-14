#include "cube.h"

//for drawing cube
glm::vec4 vertices[8] = {
    glm::vec4( -0.5, -0.5,  0.5, 1.0 ),
    glm::vec4( -0.5,  0.5,  0.5, 1.0 ),
    glm::vec4(  0.5,  0.5,  0.5, 1.0 ),
    glm::vec4(  0.5, -0.5,  0.5, 1.0 ),
    glm::vec4( -0.5, -0.5, -0.5, 1.0 ),
    glm::vec4( -0.5,  0.5, -0.5, 1.0 ),
    glm::vec4(  0.5,  0.5, -0.5, 1.0 ),
    glm::vec4(  0.5, -0.5, -0.5, 1.0 )
};
// RGBA olors
glm::vec4 vertex_colors[8] = {
    glm::vec4( 0.0, 0.0, 0.0, 1.0 ),  // black
    glm::vec4( 0.0, 1.0, 1.0, 1.0 ),  // cyan
    glm::vec4( 1.0, 0.0, 1.0, 1.0 ),  // magenta
    glm::vec4( 1.0, 1.0, 0.0, 1.0 ),  // yellow
    glm::vec4(1.0, 0.0, 0.0, 1.0 ),  // red
    glm::vec4( 0.0, 1.0, 0.0, 1.0 ),  // green
    glm::vec4( 0.0, 0.0, 1.0, 1.0 ),  // blue
    glm::vec4( 1.0, 1.0, 1.0, 1.0 )  // white
};

Cube::Cube(unsigned int programID, glm::vec3 size){

    isStatic = true;
    setCollider(new BoxCollider(size));

    std::vector<glm::vec4> points;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;
    points.reserve(36);
    colors.reserve(36);
    normals.reserve(36);

    colorcube(points, colors, normals);

    setupMesh(programID, points, colors, normals);
}

// generate 12 triangles: 36 vertices, 36 colors, 36 normals
void Cube::colorcube(std::vector<glm::vec4>& points, std::vector<glm::vec4>& colors, std::vector<glm::vec3>& normals) {
    quad( points, colors, normals, 1, 0, 3, 2 );
    quad( points, colors, normals, 2, 3, 7, 6 );
    quad( points, colors, normals, 3, 0, 4, 7 );
    quad( points, colors, normals, 6, 5, 1, 2 );
    quad( points, colors, normals, 4, 5, 6, 7 );
    quad( points, colors, normals, 5, 4, 0, 1 );
}

// quad generates two triangles for each face and assigns colors and the face normal
//    to the vertices
void Cube::quad( std::vector<glm::vec4>& points, std::vector<glm::vec4>& colors, std::vector<glm::vec3>& normals,
                 int a, int b, int c, int d ) {
    // 면의 법선: 두 모서리 벡터의 외적. 이 정점 순서에서는 큐브 바깥쪽을 향한다.
    // 꼭짓점을 면마다 복제해서 쓰기 때문에, 면마다 다른 법선을 줄 수 있어 각진 모양이 제대로 나온다
    glm::vec3 normal = glm::normalize(glm::cross(glm::vec3(vertices[b] - vertices[a]),
                                                 glm::vec3(vertices[c] - vertices[a])));

    int order[6] = { a, b, c, a, c, d };
    for (int i : order) {
        points.push_back(vertices[i]);
        colors.push_back(vertex_colors[i]);
        normals.push_back(normal);
    }
}
