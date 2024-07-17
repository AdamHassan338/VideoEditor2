#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <map>
#include <string>
#include "video.h"


class ResourceManager
{
public:
    static ResourceManager& getInstance();

    ResourceManager(const ResourceManager*) = delete;
    ResourceManager& operator =(const ResourceManager&) = delete;

    Video* loadVideo(std::string path);

    struct ManagedVideo{
        Video* video;
        unsigned int users;

        ManagedVideo(Video* video) : video(video), users(1){

        }
    };



private:
    ResourceManager() = default;
    quint64 m_nextId =0;
    std::map<std::string,ManagedVideo> m_videos;



};

#endif // RESOURCEMANAGER_H
