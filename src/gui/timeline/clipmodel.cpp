#include "clipmodel.h"
#include "video.h"

ClipModel::ClipModel(int pos, int in, int out,Video* video,int streamIndex, TrackModel* parent,MediaType type) : m_pos(pos),m_in(in),m_out(out), m_video(video), m_streamIndex(streamIndex), m_parent(parent),m_length(out),m_type(type)
{

}

int ClipModel::pos() const
{
    return m_pos;
}

void ClipModel::setPos(int newPos)
{
    m_pos = newPos;
}

void ClipModel::setParent(TrackModel *newParent)
{
    m_parent = newParent;
}

int ClipModel::in() const
{
    return m_in;
}

void ClipModel::setIn(int newIn)
{
    m_in = newIn;
}

int ClipModel::out() const
{
    return m_out;
}

void ClipModel::setOut(int newOut)
{
    m_out = newOut;
}

int ClipModel::length() const
{
    return m_length;
}

MediaType ClipModel::type() const
{
    return m_type;
}

Video *ClipModel::video() const
{
    return m_video;
}

int ClipModel::streamIndex() const
{
    return m_streamIndex;
}
