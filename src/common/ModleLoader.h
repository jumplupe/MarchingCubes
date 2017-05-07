/*
 * ModleLoader.h
 *
 *  Created on: 2017年5月3日
 *      Author: 张璐鹏
 */

#ifndef MODLELOADER_H_
#define MODLELOADER_H_

bool loadOBJ(const char* path, std::vector<glm::vec3>&out_vertices,
		std::vector<glm::vec3>&out_uvs, std::vector<glm::vec3>&out_normals);

#endif /* MODLELOADER_H_ */
