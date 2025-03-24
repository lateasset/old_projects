/**
 *   #, #,         CCCCCC  VV    VV MM      MM RRRRRRR
 *  %  %(  #%%#   CC    CC VV    VV MMM    MMM RR    RR
 *  %    %## #    CC        V    V  MM M  M MM RR    RR
 *   ,%      %    CC        VV  VV  MM  MM  MM RRRRRR
 *   (%      %,   CC    CC   VVVV   MM      MM RR   RR
 *     #%    %*    CCCCCC     VV    MM      MM RR    RR
 *    .%    %/
 *       (%.      Computer Vision & Mixed Reality Group
 *                For more information see <http://cvmr.info>
 *
 * This file is part of RBOT.
 *
 *  @copyright:   RheinMain University of Applied Sciences
 *                Wiesbaden Rüsselsheim
 *                Germany
 *     @author:   Henning Tjaden
 *                <henning dot tjaden at gmail dot com>
 *    @version:   1.0
 *       @date:   30.08.2018
 *
 * RBOT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RBOT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RBOT. If not, see <http://www.gnu.org/licenses/>.
 */

#include "model.h"
#include "tclc_histograms.h"

#include <limits>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>

using namespace std;
using namespace cv;

Model::Model(const string modelFilename, float tx, float ty, float tz, float alpha, float beta, float gamma, float scale)
{
    m_id = 0;
    
    initialized = false;
    
    buffersInitialsed = false;
    
    T_i = Transformations::translationMatrix(tx, ty, tz)
    *Transformations::rotationMatrix(alpha, Vec3f(1, 0, 0))
    *Transformations::rotationMatrix(beta, Vec3f(0, 1, 0))
    *Transformations::rotationMatrix(gamma, Vec3f(0, 0, 1))
    *Matx44f::eye();
    
    T_cm = T_i;
    
    scaling = scale;
    
    T_n = Matx44f::eye();
    
    hasNormals = false;
    
    loadModel(modelFilename);
}


Model::~Model()
{
    vertices.clear();
    normals.clear();
    
    indices.clear();
    offsets.clear();
}

void Model::initBuffers()
{
    meshes->setupMesh();
}


void Model::initialize()
{
    initialized = true;
}


bool Model::isInitialized()
{
    return initialized;
}


void Model::draw(Shader *program, GLint primitives)
{
    meshes->Draw();
}


Matx44f Model::getPose()
{
    return T_cm;
}

void Model::setPose(const Matx44f &T_cm)
{
    this->T_cm = T_cm;
}

void Model::setInitialPose(const Matx44f &T_cm)
{
    T_i = T_cm;
}

Matx44f Model::getNormalization()
{
    return T_n;
}


Vec3f Model::getLBN()
{
    return lbn;
}

Vec3f Model::getRTF()
{
    return rtf;
}

float Model::getScaling() {
    
    return scaling;
}


vector<Vec3f> Model::getVertices()
{
    return vertices;
}

int Model::getNumVertices()
{
    return (int)vertices.size();
}


int Model::getModelID()
{
    return m_id;
}


void Model::setModelID(int i)
{
    m_id = i;
}


void Model::reset()
{
    initialized = false;
    
    T_cm = T_i;
}


void Model::loadModel(const string modelFilename)
{
    Assimp::Importer importer;
    
    const aiScene* scene = importer.ReadFile(modelFilename, aiProcessPreset_TargetRealtime_Fast);
    
    aiMesh *mesh = scene->mMeshes[0];
    
    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);
    
    hasNormals = mesh->HasNormals();
    
    float inf = numeric_limits<float>::infinity();
    lbn = Vec3f(inf, inf, inf);
    rtf = Vec3f(-inf, -inf, -inf);
    
    for(int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace f = mesh->mFaces[i];
        
        indices.push_back(f.mIndices[0]);
        indices.push_back(f.mIndices[1]);
        indices.push_back(f.mIndices[2]);
    }
    
    for(int i = 0; i < mesh->mNumVertices; i++)
    {
        aiVector3D v = mesh->mVertices[i];
        
        Vec3f p(v.x, v.y, v.z);
        
        // compute the 3D bounding box of the model
        if (p[0] < lbn[0]) lbn[0] = p[0];
        if (p[1] < lbn[1]) lbn[1] = p[1];
        if (p[2] < lbn[2]) lbn[2] = p[2];
        if (p[0] > rtf[0]) rtf[0] = p[0];
        if (p[1] > rtf[1]) rtf[1] = p[1];
        if (p[2] > rtf[2]) rtf[2] = p[2];
        
        vertices.push_back(p);
    }
    
    if(hasNormals)
    {
        for(int i = 0; i < mesh->mNumVertices; i++)
        {
            aiVector3D n = mesh->mNormals[i];
            
            Vec3f vn = Vec3f(n.x, n.y, n.z);
            
            normals.push_back(vn);
        }
    }
    
    offsets.push_back(0);
    offsets.push_back(mesh->mNumFaces*3);
    
    // the center of the 3d bounding box
    Vec3f bbCenter = (rtf + lbn)/2;
    
    // compute a normalization transform that moves the object to the center of its bounding box and scales it according to the prescribed factor
    T_n = Transformations::scaleMatrix(scaling)*Transformations::translationMatrix(-bbCenter[0], -bbCenter[1], -bbCenter[2]);
    
    //T_n = Transformations::scaleMatrix(scaling);
}


// processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
void Model::processNode(aiNode* node, const aiScene* scene)
{
    // process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene.
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        
        for(int i = 0; i < mesh->mNumVertices; i++)
        {
            aiVector3D v = mesh->mVertices[i];
            
            Vec3f p(v.x, v.y, v.z);
            
            // compute the 3D bounding box of the model
            if (p[0] < lbn[0]) lbn[0] = p[0];
            if (p[1] < lbn[1]) lbn[1] = p[1];
            if (p[2] < lbn[2]) lbn[2] = p[2];
            if (p[0] > rtf[0]) rtf[0] = p[0];
            if (p[1] > rtf[1]) rtf[1] = p[1];
            if (p[2] > rtf[2]) rtf[2] = p[2];
            
            vertices.push_back(p);
        }
        
        if(hasNormals)
        {
            for(int i = 0; i < mesh->mNumVertices; i++)
            {
                aiVector3D n = mesh->mNormals[i];
                
                Vec3f vn = Vec3f(n.x, n.y, n.z);
            }
        }
        
        meshes = processMesh(mesh, scene);
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
    cout << "Node" << endl;
}

Mesh* Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    // data to fill
    vector<Vertex> vertexes; // CHANGE
    vector<unsigned int> indices;

    // walk through each of the mesh's vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
        // positions
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;
        // normals
        if (mesh->HasNormals())
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }
        // texture coordinates
        if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            glm::vec2 vec;
            // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
            // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
            // tangent
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangent = vector;
            // bitangent
            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangent = vector;
        }
        else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);

        vertexes.push_back(vertex);
    }
    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    // process materials
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    // return a mesh object created from the extracted mesh data
    return new Mesh(vertexes, indices);
    cout << "Model" << endl;
}
