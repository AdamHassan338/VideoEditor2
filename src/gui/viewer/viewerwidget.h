#ifndef VIEWERWIDGET_H
#define VIEWERWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "video.h"

class QPushButton;
class QPixmap;
class QLabel;

class ViewerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ViewerWidget(QWidget *parent = nullptr);
    void setSrcRes(int width, int height);

signals:
    void play();
    void pause();

public slots:
    void setImage(VideoFrame frame);


private:
    QLabel* m_label;
    QPixmap* m_pixmap;
    QPushButton* m_playButton;
    int m_srcWidth = 1920;
    int m_srcHeight = 1080;
    QHBoxLayout *viewerlayout;
    QVBoxLayout* layout;
    QImage m_lastImage;
    VideoFrame lastFrame;

    void scalePixmap();

private slots:
        void playPause(bool b);

    // QWidget interface
protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif // VIEWERWIDGET_H
