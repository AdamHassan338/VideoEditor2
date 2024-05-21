#include "resourcemanager.h"
#include <string.h>

ResourceManager& ResourceManager::getInstance()
{
    static ResourceManager instance;
    return instance;
}

Video* ResourceManager::loadVideo(std::string path)
{
    std::map<std::string,ManagedVideo>::iterator it = m_videos.find(path);
    if(it != m_videos.end()){
        it->second.users ++;
        //std::cout << it->second.users << std::endl;
        return it->second.video;

    }

    ManagedVideo newVideo = ManagedVideo(new Video(path.c_str()));
    if(!newVideo.video->open())
        return nullptr;
    m_videos.insert(std::pair(path,newVideo));

    if(newVideo.video){
        return newVideo.video;
    }
    return nullptr;
}
