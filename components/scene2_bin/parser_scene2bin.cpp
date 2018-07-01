#include <scene2_bin/parser_scene2bin.hpp>

namespace MFFormat
{

std::string DataFormatScene2BIN::lightTypeToStr(LightType t)
{
    switch (t)
    {
        case MFFormat::DataFormatScene2BIN::LIGHT_TYPE_POINT: return "point"; break;
        case MFFormat::DataFormatScene2BIN::LIGHT_TYPE_DIRECTIONAL: return "directional"; break;
        case MFFormat::DataFormatScene2BIN::LIGHT_TYPE_AMBIENT: return "ambient"; break;
        case MFFormat::DataFormatScene2BIN::LIGHT_TYPE_FOG: return "fog"; break;
        case MFFormat::DataFormatScene2BIN::LIGHT_TYPE_POINT_AMBIENT: return "point ambient"; break;
        case MFFormat::DataFormatScene2BIN::LIGHT_TYPE_LAYERED_FOG: return "layered fog"; break;
        default: break;
    }

    return "unknown";
}

bool DataFormatScene2BIN::load(std::ifstream &srcFile)
{
    Header newHeader = {};
    read(srcFile, &newHeader);
    uint32_t position = 6;

    while(position + 6 < newHeader.mSize)
    {
        srcFile.seekg(position, srcFile.beg);
        Header nextHeader = {};
        read(srcFile, &nextHeader);
        readHeader(srcFile, &nextHeader, position + 6);
        position += nextHeader.mSize;
    }

    return true;
}

void DataFormatScene2BIN::readHeader(std::ifstream &srcFile, Header* header, uint32_t offset)
{
    switch(header->mType)
    {
        case HEADER_SPECIAL_WORLD:
        case HEADER_WORLD:
        {
            uint32_t position = offset;
            while(position + 6 < offset + header->mSize)
            {
                Header nextHeader = {};
                srcFile.seekg(position, srcFile.beg);
                read(srcFile, &nextHeader);
                readHeader(srcFile, &nextHeader, position + 6);
                position += nextHeader.mSize;
            }
        }
        break;

        case HEADER_VIEW_DISTANCE:
        {
            read(srcFile, &mViewDistance);
        } 
        break;

        case HEADER_CLIPPING_PLANES:
        {
            read(srcFile, &mClippingPlanes);
        }
        break;

        case HEADER_FOV:
        {
            read(srcFile, &mFov);
        } 
        break;

        case HEADER_SPECIAL_OBJECT:
        case HEADER_OBJECT:
        {
            uint32_t position = offset;
            Object newObject = {};
            while(position + 6 < offset + header->mSize)
            {
                Header nextHeader = {};
                srcFile.seekg(position, srcFile.beg);
                read(srcFile, &nextHeader);
                readObject(srcFile, &nextHeader, &newObject, position + 6);
                position += nextHeader.mSize;
            }

            if (header->mType == HEADER_OBJECT) {
                mObjects.insert(make_pair(newObject.mName, newObject));
            }
            else {
                auto object_it = mObjects.find(newObject.mName);

                if (object_it != mObjects.end()) {
                    auto object = &object_it->second;
                    object->mSpecialType = newObject.mSpecialType;

                    memcpy(&object->mSpecialProps, &newObject.mSpecialProps, sizeof(newObject.mSpecialProps));
                }
            }
        } 
        break;
    }
}

void DataFormatScene2BIN::readObject(std::ifstream &srcFile, Header* header, Object* object, uint32_t offset)
{
    switch(header->mType)
    {
        case OBJECT_TYPE_SPECIAL:
        {
            read(srcFile, &object->mSpecialType);
        }
        break;

        case OBJECT_TYPE_NORMAL:
        {
            read(srcFile, &object->mType);
        }
        break;

        case OBJECT_NAME:
        case OBJECT_NAME_SPECIAL:
        {
            char *name = reinterpret_cast<char*>(malloc(header->mSize + 1));
            read(srcFile, name, header->mSize - 1);
            name[header->mSize] = '\0';

            object->mName = std::string(name);
            free(name);
        }
        break;

        case OBJECT_SPECIAL_DATA:
        {
            switch (object->mSpecialType) {
                case SPECIAL_OBJECT_TYPE_PHYSICAL:
                {
                    srcFile.seekg(2, srcFile.cur);

                    read(srcFile, &object->mSpecialProps.mMovVal1);
                    read(srcFile, &object->mSpecialProps.mMovVal2);
                    read(srcFile, &object->mSpecialProps.mWeight);
                    read(srcFile, &object->mSpecialProps.mFriction);
                    read(srcFile, &object->mSpecialProps.mMovVal4);
                    read(srcFile, &object->mSpecialProps.mSound);
                    
                    srcFile.seekg(1, srcFile.cur);

                    read(srcFile, &object->mSpecialProps.mMovVal5);
                }
                break;
            }
        }
        break;

        case OBJECT_MODEL:
        {
            char *modelName = reinterpret_cast<char*>(malloc(header->mSize + 1));
            read(srcFile, modelName, header->mSize);
            modelName[strlen(modelName) - 4] = '\0';
            sprintf(modelName, "%s.4ds", modelName);
            modelName[header->mSize] = '\0';

            object->mModelName = std::string(modelName);
            free(modelName);
        }
        break;

        case OBJECT_POSITION:
        {
            MFMath::Vec3 newPosition = {};
            read(srcFile, &newPosition);
            object->mPos = newPosition;
        } 
        break;

        case OBJECT_ROTATION:
        {
            MFMath::Quat newRotation = {};
            read(srcFile, &newRotation);
            newRotation.fromMafia();
            object->mRot = newRotation;
        } 
        break;

        case OBJECT_POSITION_2:
        {
            MFMath::Vec3 newPosition = {};
            read(srcFile, &newPosition);
            object->mPos2 = newPosition;
        } 
        break;

        case OBJECT_SCALE:
        {
            MFMath::Vec3 newScale = {};
            read(srcFile, &newScale);
            object->mScale = newScale;
        } 
        break;

        case OBJECT_LIGHT_MAIN:
        {
            uint32_t position = offset;
            while (position + 6 < offset + header->mSize)
            {
                Header lightHeader = {};
                read(srcFile, &lightHeader);
                readLight(srcFile, &lightHeader, object);
                position += lightHeader.mSize;
            }
        }
        break;

        case OBJECT_PARENT:
        {
            Header parentHeader = {};
            read(srcFile, &parentHeader);
            Object parentObject = {};
            readObject(srcFile, &parentHeader, &parentObject, offset + 6);

            object->mParentName = parentObject.mName;
        }
        break;

        case OBJECT_LIGHT_MAP:
        {
            uint8_t bitmapOfLevelOfDetails;
            read(srcFile, &bitmapOfLevelOfDetails);

            // if no lightmap is present for this object
            if(bitmapOfLevelOfDetails == 0)
                break;

            unsigned char countOfLightmaps = 0;
            // count the number of lightmap levels
            uint8_t temporaryBitmap = bitmapOfLevelOfDetails;
            std::cout << "Bitmap: ";
            while(temporaryBitmap!= 0)
            {
                std::cout << (int) (temporaryBitmap & 1);
                if((temporaryBitmap & 1) == 1)
                    countOfLightmaps++;
                temporaryBitmap >>=1;
            }
            std::cout << "\n";

            std::cout << "Count of ligtmaps: " << (int) countOfLightmaps << "\n";
            for(unsigned char i = 0; i < countOfLightmaps; i++)
            {
                // read general data which are common for both BMP / vertex lightmaps
                LightmapGeneralData generalHeader;
                read(srcFile, &generalHeader);
                
                std::cout << "Obj. lightmap starting offset: " <<  srcFile.tellg() << "\n";
                std::cout << "\tGeneral Section:" << "\n";
                std::cout << "\tLightmap type:\t" << (int) generalHeader.mTypeOfLightmap<< "\n";
                std::cout << "\tLightmap no. parts:\t" << (int) generalHeader.mNuberOfParts<< "\n";
                std::cout << "\tLightmap level ID:\t" << (int) generalHeader.mLevelId << "\n";
                std::cout << "\tFloat A:\t" << generalHeader.mUnkA<< "\n";
                std::cout << "\tFloat B:\t" << generalHeader.mUnkB<< "\n";



                for(unsigned part = 0; part < generalHeader.mNuberOfParts; part++)
                {
                    uint16_t unkShit;
                    read(srcFile, &unkShit);

                    switch(generalHeader.mTypeOfLightmap)
                    {


                        case LIGHTMAP_TYPE_VERTEX:
                        {
                                std::cout << "Obj. lightmap starting offset: " <<  srcFile.tellg() << "\n";
                                uint32_t numberOfVertices;
                                read(srcFile, &numberOfVertices);
                                std::cout << "\tVertices: \t" << numberOfVertices << "\n";
                                

                                
                                uint32_t* arrayofRGBA = new uint32_t[numberOfVertices];
                                read(srcFile, arrayofRGBA, sizeof(uint32_t)*numberOfVertices);
                                //srcFile.read((char*)arrayofRGBA, (sizeof(uint32_t)*numberOfVertices));
                                
                        }
                        break;
                        case LIGHTMAP_TYPE_MAP:
                        {
                            uint16_t numberOfVertices;
                            read(srcFile, &numberOfVertices);
                            uint16_t numberOfFacets;
                            read(srcFile, &numberOfFacets);

                            std::cout << "\tVerties: \t" << numberOfVertices << "\n";
                            std::cout << "\tFacets: \t" << numberOfFacets<< "\n";

                            uint8_t flagIsDwordPresent;
                            read(srcFile, &flagIsDwordPresent);

                            std::cout << "\tFlag: \t" << (int) flagIsDwordPresent << "\n";

                            uint32_t unkDword;
                            if(flagIsDwordPresent)
                                read(srcFile, &unkDword);


                            uint32_t countOfSomething;
                            read(srcFile, &countOfSomething);

                            std::cout << "\tCount: \t" << (int) countOfSomething<< "\n";


                            for(unsigned s = 0; s < countOfSomething; s++)
                            {
                                std::cout << "Float pos>>: " <<  srcFile.tellg() << "\n";
                                uint32_t sizeA;
                                read(srcFile, &sizeA);
                                uint32_t sizeB;
                                read(srcFile, &sizeB);
                                std::cout << "\tSize: [" << (int) sizeA << "," << sizeB << "]\n";

                                uint32_t sizeOfArray = sizeA*sizeB*3;
                                uint8_t* arrayofRGB = new uint8_t[sizeOfArray];
                                read(srcFile, arrayofRGB, sizeOfArray);

                            }

                            uint32_t countOfUVCoordinates; 
                            read(srcFile, &countOfUVCoordinates);
                            std::cout << "\tCount of UV coordinates: \t" << (int) countOfUVCoordinates<< "\n";
                            
                        }
                        break;
                    }
                }
                return;

            }
            return;
        
            uint16_t numberOfVertices;
            read(srcFile, &numberOfVertices);

            uint16_t numberOfLBMP;
            read(srcFile, &numberOfLBMP);

            uint16_t typeOfFollowing;
            read(srcFile, &typeOfFollowing);


            std::cout << "\t" << "Section2:\n";
            std::cout << "\t\t" << "noVert:\t"<<numberOfVertices<<"\n";
            std::cout << "\t\t" << "noLBMP:\t"<<numberOfLBMP<<"\n";
            std::cout << "\t\t" << "typeFollows:\t"<<typeOfFollowing<<"\n";

            uint8_t hasDword;
            read(srcFile, &hasDword);
			
			
            uint32_t countOfSomething;
            read(srcFile, &countOfSomething);

            uint32_t sizeA;
            read(srcFile, &sizeA);

            uint32_t sizeB;
            read(srcFile, &sizeB);

		
            std::cout << "\t\t" << "Flag:\t"<<(int) hasDword<<"\n";
            std::cout << "\t\t" << "SizeA:\t"<<sizeA<<"\n";
            std::cout << "\t\t" << "SizeB:\t"<<sizeB<<"\n";
            
        }
        break;


    }
}

void DataFormatScene2BIN::readLight(std::ifstream &srcFile, Header* header, Object* object)
{
    switch(header->mType)
    {
        case OBJECT_LIGHT_TYPE:
        {
            read(srcFile, &object->mLightType);
        }
        break;
        
        case OBJECT_LIGHT_COLOUR:
        {
            read(srcFile, &object->mLightColour);
        }
        break;
        
        case OBJECT_LIGHT_POWER:
        {
            read(srcFile, &object->mLightPower);
        }
        break;
        
        case OBJECT_LIGHT_RANGE:
        {
            read(srcFile, &object->mLightNear);
            read(srcFile, &object->mLightFar);
        }
        break;
        
        case OBJECT_LIGHT_SECTOR:
        {
            //read(srcFile, object->mLightSectors, header->mSize);
        }
        break;
        
        case OBJECT_LIGHT_FLAGS:
        {
            read(srcFile, &object->mLightFlags);
        }
        break;

        case OBJECT_LIGHT_UNK_1:
        {
            read(srcFile, &object->mLightUnk0);
            read(srcFile, &object->mLightUnk1);
        }
        break;
    }
}

}
