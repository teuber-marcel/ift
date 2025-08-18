#ifndef MARKER_H
#define MARKER_H

#include "global.h"
#include <views/orientation.h>
#include <QVector>
#include <markerinformation.h>
#include <segmentation/forestwrapper.h>

#define MARKER_INSERTION 1
#define MARKER_REMOVAL   2

class Marker : public QObject
{
    Q_OBJECT
public:
    Marker(iftImageSequence *img);
    ~Marker();

     iftImage *getMarkers();
    void setMarkers( iftImage *img);

    void resetMarkers();

    void setBrushRadius(float new_radius);
    float getBrushRadius();
    iftAdjRel *getBrush();
    void setSphericity(bool sphere);

    void setAnnotationMode(char mode);
    char getAnnotationMode();

    void paintMarkersOnImage(Orientation *orientation, iftImage *img, QPoint point, int markerLabel, int sliceType, int sliceNum, bool erasing);
    void paintMarkersOnQImage(Orientation *orientation, QImage *img, QPoint point, int markerLabel, int sliceType, int sliceNum, bool erasing);
    void selectPointForDeletion(iftVoxel u, iftImage *label, ForestWrapper *forest);

    void paintMarker(iftVoxel v, int label);
    void paintBox(iftVoxel v, int label);
    void eraseMarker(iftVoxel v);
    void setMarkerVisibility(int marker, int visibility);

    iftColor getMarkerColor(int marker);    
    //TODO make const
    MarkerInformationVector& getMarkerInfoArray();
    void setMarkerName(int index, QString name);
    int getIndexFromLabel(int label);
    void setMarkerLabel(int index, int label);
    iftColorTable *generateColorTable();//TODO getColorTable();
    iftIntArray *generateMarkersVisibility();

    void addItemInColorTable(int label = -1, iftColor *c = nullptr);
    void setObjectColorInColorTable(int obj, iftColor color);
    void destroyColorTable();

    void removeMarker(int markerLabel);

    void setLastPoint(QPoint *newPoint);
    const QPoint* getLastPoint() const;

     iftImage *getRemovalMarkers();

    int getNextLabel();
    int getLabel(int index);

    void undo();
private:

    /******** IFT structures ********/
    iftImage *markers;
    iftAdjRel *brush;
    iftBoundingBox bb;

    QVector<QPair<int, iftLabeledSet*>> historic;

    MarkerInformationVector markerInfoArray;

    iftImage *removalMarkers;

    /******** Variables ********/
    float radius;
    bool sphere;
    char annotationMode;
    int bbIndex;
    int nmarkers;

    QPoint *lastPoint;
};

#endif // MARKER_H
