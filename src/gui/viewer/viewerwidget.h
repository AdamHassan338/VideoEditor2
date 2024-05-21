#ifndef VIEWERWIDGET_H
#define VIEWERWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

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

private:
    QLabel* m_label;
    QPixmap* m_pixmap;
    QPushButton* m_playButton;
    int m_srcWidth = 1920;
    int m_srcHeight = 1080;
    QHBoxLayout *viewerlayout;
    QVBoxLayout* layout;

    void scalePixmap();


    // QWidget interface
protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif // VIEWERWIDGET_H
