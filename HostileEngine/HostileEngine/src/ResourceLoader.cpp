#include <stdafx.h>
#include "ResourceLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <array>
#include <map>
#include <memory>



namespace Hostile
{
    bool Init(ComPtr<ID3D12Device>& _device)
    {
        return false;
    }
    
    void BuildSkeletonHeirarchy(aiNode* _pNode, Skeleton::Node* _node)
    {
        _node->name = _pNode->mName.C_Str();
        memcpy(&_node->transformation, &_pNode->mTransformation, sizeof(Matrix));
        //_node->transformation = _node->transformation.Transpose();
        _node->children.resize(_pNode->mNumChildren);
        for (int i = 0; i < _pNode->mNumChildren; i++)
        {
            BuildSkeletonHeirarchy(_pNode->mChildren[i], &_node->children[i]);
        }
    }
    
    SceneData LoadSceneFromFile(std::string&& _filepath)
    {
        Assimp::Importer importer;
        
        const aiScene* pScene = importer.ReadFile(
            _filepath,
            aiProcess_OptimizeMeshes  |
            aiProcess_Triangulate     |
            aiProcess_FlipUVs         |
            aiProcess_FlipWindingOrder|
            aiProcess_LimitBoneWeights
        );
        
       const char* error = importer.GetErrorString();
        
       std::cout << _filepath << std::endl;
       aiMetadata* pMetaData = pScene->mMetaData;
       for (uint32_t i = 0; i < pMetaData->mNumProperties; i++)
       {
           std::string name = pMetaData->mKeys[i].C_Str();
           aiMetadataEntry entry = pMetaData->mValues[i];
           switch (entry.mType)
           {
           case aiMetadataType::AI_INT32:
           {
               int stuff;
               pMetaData->Get<int>(name, stuff);
               std::cout << name << ": " << stuff << std::endl;
           }
               break;
           case aiMetadataType::AI_BOOL:
           {
               bool stuff;
               pMetaData->Get<bool>(name, stuff);
               std::cout << name << ": " << stuff << std::endl;
           }
           break;
           case aiMetadataType::AI_UINT64:
           {
               uint64_t stuff;
               pMetaData->Get<uint64_t>(name, stuff);
               std::cout << name << ": " << stuff << std::endl;
           }
           break;

           case aiMetadataType::AI_FLOAT:
           {
               float stuff;
               pMetaData->Get<float>(name, stuff);
               std::cout << name << ": " << stuff << std::endl;
           }
           break;

           case aiMetadataType::AI_DOUBLE:
           {
               double stuff;
               pMetaData->Get<double>(name, stuff);
               std::cout << name << ": " << stuff << std::endl;
           }
           break;

           case aiMetadataType::AI_AISTRING:
           {
               aiString stuff;
               pMetaData->Get<aiString>(name, stuff);
               std::cout << name << ": " << stuff.C_Str() << std::endl;
           }
           break;

           case aiMetadataType::AI_AIVECTOR3D:
           {
               aiVector3D stuff;
               pMetaData->Get<aiVector3D>(name, stuff);
               std::cout << name << ": (" << stuff.x << ", " << stuff.y << ", " << stuff.z << ")\n";
           }
           break;

           case aiMetadataType::AI_AIMETADATA:
           {
               //int stuff;
               //pMetaData->Get<int>(name, stuff);
               std::cout << name << ": " << "metadata";
           }
           break;
           }
       }

       std::cout << std::endl;
        SceneData scene;
        if (pScene->HasMeshes())
        {
            //MeshData mesh;
            memcpy(&scene.skeleton.globalInverseTransform, &pScene->mRootNode->mTransformation, sizeof(Matrix));
            //scene.skeleton.globalInverseTransform = scene.skeleton.globalInverseTransform.Transpose();
            scene.skeleton.globalInverseTransform = scene.skeleton.globalInverseTransform.Invert();
            for (int i = 0; i < pScene->mNumMeshes; i++)
            {
                aiMesh* pMesh = pScene->mMeshes[i];
                //scene.meshData.entries.push_back(MeshData::Entry{ pMesh->mName.C_Str(), scene.meshData.vertices.size() });
                int numColorChannels = pMesh->GetNumColorChannels();
                int numUVChannels = pMesh->GetNumUVChannels();
                
        
                scene.meshData.entries.push_back({ pMesh->mName.C_Str(), scene.meshData.vertices.size() });
                if (pMesh->HasPositions())
                {
                    for (int p = 0; p < pMesh->mNumVertices; p++)
                    {
                        aiVector3D& v = pMesh->mVertices[p];
                        MeshData::VertexBone vertex{};
                        vertex.pos = { v.x, v.y, v.z };
                        scene.meshData.vertices.push_back(vertex);
                    }
                }
        
                if (pMesh->HasBones())
                {
                    //mesh.weights.resize(pMesh->mNumVertices + mesh.entries[i].baseVertex);
                    //mesh.boneIds.resize(pMesh->mNumVertices + mesh.entries[i].baseVertex);
   
                    for (int bone = 0; bone < pMesh->mNumBones; bone++)
                    {
                        const aiBone* pBone = pMesh->mBones[bone];
                        
                        for (int w = 0; w < pBone->mNumWeights; w++)
                        {
                            scene.meshData
                                .vertices[pBone->mWeights[w].mVertexId + scene.meshData.entries[i].baseVertex]
                                .AddBone(bone, pBone->mWeights[w].mWeight);
                        }
                        Skeleton::Bone b{};
                        size_t boneIndex = bone;
                        scene.skeleton.boneMapping.insert({ pBone->mName.C_Str(), boneIndex });
                        memcpy(&b.offset, &pBone->mOffsetMatrix, sizeof(Matrix));
                        //b.offset = b.offset.Transpose();
                        b.finalTransform = Matrix::Identity;
                        b.name = pBone->mName.C_Str();
                        scene.skeleton.bones.push_back(b);
                    }
                }
        
                if (pMesh->HasFaces())
                {
                    for (int f = 0; f < pMesh->mNumFaces; f++)
                    {
                        scene.meshData.faces.push_back(pMesh->mFaces[f].mIndices[0]+ scene.meshData.entries[i].baseVertex);
                        scene.meshData.faces.push_back(pMesh->mFaces[f].mIndices[1]+ scene.meshData.entries[i].baseVertex);
                        scene.meshData.faces.push_back(pMesh->mFaces[f].mIndices[2]+ scene.meshData.entries[i].baseVertex);
                    }
                }
        
                if (pMesh->HasNormals())
                {
                    for (int n = 0; n < pMesh->mNumVertices; n++)
                    {
                        const aiVector3D& v = pMesh->mNormals[n];
                        scene.meshData.vertices[n + scene.meshData.entries[i].baseVertex].normal = { v.x, v.y, v.z };
                    }
                }
        
                if (numUVChannels > 0)
                {
                    for (int u = 0; u < numUVChannels; u++)
                    {
                        for (int j = 0; j < pMesh->mNumVertices; j++)
                        {
                            scene.meshData.vertices[j + scene.meshData.entries[i].baseVertex].texCoord = {
                                pMesh->mTextureCoords[0][j].x,
                                pMesh->mTextureCoords[0][j].y
                            };
                        }
                    }
                }
            }
            BuildSkeletonHeirarchy(pScene->mRootNode, &scene.skeleton.rootNode);
        }
        
        
        if (pScene->HasAnimations())
        {
            
            for (int a = 0; a < pScene->mNumAnimations; a++)
            {
                aiAnimation* pAnim = pScene->mAnimations[a];
                Animation anim;
                anim.duration = pAnim->mDuration;
                anim.ticksPerSec = pAnim->mTicksPerSecond;
                anim.numChannels = pAnim->mNumChannels;
                anim.name = pAnim->mName.C_Str();
        
                for (int n = 0; n < pAnim->mNumChannels; n++)
                {
                    aiNodeAnim* pNode = pAnim->mChannels[n];
                    AnimationNode node;
                    node.nodeName = pNode->mNodeName.C_Str();
                    
                    for (int c = 0; c < pNode->mNumPositionKeys; c++)
                    {
                        aiVectorKey vKey = pNode->mPositionKeys[c];
                        node.positionKeys.push_back({ vKey.mTime, 
                            { 
                                (vKey.mValue.x == -0.0f) ? 0 : vKey.mValue.x, 
                                (vKey.mValue.y == -0.0f) ? 0 : vKey.mValue.y,
                                (vKey.mValue.z == -0.0f) ? 0 : vKey.mValue.z
                            } });
                    }
        
                    for (int c = 0; c < pNode->mNumRotationKeys; c++)
                    {
                        aiQuatKey vKey = pNode->mRotationKeys[c];
                        node.rotationKeys.push_back({ vKey.mTime, 
                            { 
                                vKey.mValue.x,
                                vKey.mValue.y,
                                vKey.mValue.z,
                                vKey.mValue.w
                            } });
                    }
                    
                    for (int c = 0; c < pNode->mNumScalingKeys; c++)
                    {
                        aiVectorKey vKey = pNode->mScalingKeys[c];
                        node.scalingKeys.push_back({ vKey.mTime, 
                            { 
                                (vKey.mValue.x == -0.0f) ? 0 : vKey.mValue.x,
                                (vKey.mValue.y == -0.0f) ? 0 : vKey.mValue.y,
                                (vKey.mValue.z == -0.0f) ? 0 : vKey.mValue.z
                            } });
                    }
        
                    anim.nodes.push_back(node);
                }
        
                scene.animations.push_back(anim);
            }
        }
        
        std::cout << "stuff" << std::endl;
        return scene;
    }
}