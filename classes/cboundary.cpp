#include "cboundary.h"
#include "cnmea.h"
#include "math.h"
#include <QLocale>
#include <QOpenGLContext>
#include <QOpenGLFunctions_2_1>

CBoundary::CBoundary()
{

}

void CBoundary::findClosestBoundaryPoint(vec2 fromPt)
{
    boxA.easting = fromPt.easting - (Math.Sin(mf.fixHeading + glm.PIBy2) *  mf.vehicle.toolWidth * 0.5);
    boxA.northing = fromPt.northing - (Math.Cos(mf.fixHeading + glm.PIBy2)  * mf.vehicle.toolWidth * 0.5);

    boxB.easting = fromPt.easting + (Math.Sin(mf.fixHeading + glm.PIBy2) *  mf.vehicle.toolWidth * 0.5);
    boxB.northing = fromPt.northing + (Math.Cos(mf.fixHeading + glm.PIBy2)  * mf.vehicle.toolWidth * 0.5);

    boxC.easting = boxB.easting + (Math.Sin(mf.fixHeading) * 2000.0);
    boxC.northing = boxB.northing + (Math.Cos(mf.fixHeading) * 2000.0);

    boxD.easting = boxA.easting + (Math.Sin(mf.fixHeading) * 2000.0);
    boxD.northing = boxA.northing + (Math.Cos(mf.fixHeading) * 2000.0);

    //determine if point is inside bounding box
    bdList.clear();
    int ptCount = ptList.size();
    for (int p = 0; p < ptCount; p++) {
        if ((((boxB.easting - boxA.easting) * (ptList[p].northing - boxA.northing))
             - ((boxB.northing - boxA.northing) * (ptList[p].easting - boxA.easting))) < 0) {
            continue;
        }
        if ((((boxD.easting - boxC.easting) * (ptList[p].northing - boxC.northing))
             - ((boxD.northing - boxC.northing) * (ptList[p].easting - boxC.easting))) < 0) {
            continue;
        }
        if ((((boxC.easting - boxB.easting) * (ptList[p].northing - boxB.northing))
             - ((boxC.northing - boxB.northing) * (ptList[p].easting - boxB.easting))) < 0) {
            continue;
        }
        if ((((boxA.easting - boxD.easting) * (ptList[p].northing - boxD.northing))
             - ((boxA.northing - boxD.northing) * (ptList[p].easting - boxD.easting))) < 0) {
            continue;
        }

        //it's in the box, so add to list
        closestBoundaryPt.easting = ptList[p].easting;
        closestBoundaryPt.northing = ptList[p].northing;
        bdList.append(closestBoundaryPt);
    }

    //which of the points is closest
    closestBoundaryPt.easting = -1; closestBoundaryPt.northing = -1;
    ptCount = bdList.count();
    if (ptCount == 0) {
        return;
    } else {
        //determine closest point
        double minDistance = 9999999;
        for (int i = 0; i < ptCount; i++) {
            double dist = ((fromPt.easting - bdList[i].easting) * (fromPt.easting - bdList[i].easting))
                    + ((fromPt.northing - bdList[i].northing) * (fromPt.northing - bdList[i].northing));
            if (minDistance >= dist) {
                minDistance = dist;
                closestBoundaryPt = bdList[i];
            }
        }
    }
}

void CBoundary::resetBoundary()
{
    calcList.clear();
    ptList.clear();
    area = 0;
    areaAcre = 0;
    areaHectare = 0;

    isDrawRightSide = true;
    isSet = false;
    isOkToAddPoints = false;
}

void CBoundary::preCalcBoundaryLines()
{
    int j = ptList.size() - 1;
    //clear the list, constant is easting, multiple is northing
    calcList.clear();
    Vec2 constantMultiple(0, 0);

    for (int i = 0; i < ptList.size(); j = i++)
    {
        //check for divide by zero
        if (fabs(ptList[i].northing - ptList[j].northing) < 0.00000000001)
        {
            constantMultiple.easting = ptList[i].easting;
            constantMultiple.northing = 0;
        }
        else
        {
            //determine constant and multiple and add to list
            constantMultiple.easting = ptList[i].easting - (ptList[i].northing * ptList[j].easting)
                    / (ptList[j].northing - ptList[i].northing) + (ptList[i].northing * ptList[i].easting)
                    / (ptList[j].northing - ptList[i].northing);
            constantMultiple.northing = (ptList[j].easting - ptList[i].easting) / (ptList[j].northing - ptList[i].northing);
            calcList.append(constantMultiple);
        }
    }

    areaHectare = area * 0.0001;
    areaAcre = area * 0.000247105;
}

bool CBoundary::isPrePointInPolygon(Vec2 testPoint)
{
    if (calcList.size() < 10) return false;
    int j = ptList.size() - 1;
    bool oddNodes = false;

    //test against the constant and multiples list the test point
    for (int i = 0; i < ptList.size(); j = i++)
    {
        if ( ((ptList[i].northing < testPoint.northing) && (ptList[j].northing >= testPoint.northing)) ||
             ((ptList[j].northing < testPoint.northing) && (ptList[i].northing >= testPoint.northing)) )
        {
            oddNodes ^= (testPoint.northing * calcList[i].northing + calcList[i].easting < testPoint.easting);
        }
    }
    return oddNodes; //true means inside.
}

void CBoundary::drawBoundaryLine(QOpenGLContext *glContext)
{
    QOpenGLFunctions_2_1 *gl=glContext->versionFunctions<QOpenGLFunctions_2_1>();

    ////draw the perimeter line so far
    int ptCount = ptList.size();
    if (ptCount < 1) return;
    gl->glLineWidth(2);
    gl->glColor3f(0.98f, 0.2f, 0.60f);
    gl->glBegin(GL_LINE_STRIP);
    for (int h = 0; h < ptCount; h++)
        gl->glVertex3d(ptList[h].easting, ptList[h].northing, 0);
    gl->glEnd();

    //the "close the loop" line
    gl->glLineWidth(2);
    gl->glColor3f(0.9f, 0.32f, 0.70f);
    gl->glBegin(GL_LINE_STRIP);
    gl->glVertex3d(ptList[ptCount - 1].easting, ptList[ptCount - 1].northing, 0);
    gl->glVertex3d(ptList[0].easting, ptList[0].northing, 0);
    gl->glEnd();
}

//draw a blue line in the back buffer for section control over boundary line
void CBoundary::drawBoundaryLineOnBackBuffer(QOpenGLFunctions_2_1 *gl)
{

    ////draw the perimeter line so far
    int ptCount = ptList.size();
    if (ptCount < 1) return;
    gl->glLineWidth(4);
    gl->glColor3f(0.0f, 0.99f, 0.0f);
    gl->glBegin(GL_LINE_STRIP);
    for (int h = 0; h < ptCount; h++)
        gl->glVertex3d(ptList[h].easting, ptList[h].northing, 0);
    gl->glEnd();

    //the "close the loop" line
    gl->glLineWidth(4);
    gl->glColor3f(0.0f, 0.990f, 0.0f);
    gl->glBegin(GL_LINE_STRIP);
    gl->glVertex3d(ptList[ptCount - 1].easting, ptList[ptCount - 1].northing, 0);
    gl->glVertex3d(ptList[0].easting, ptList[0].northing, 0);
    gl->glEnd();
}

void CBoundary::calculateBoundaryArea()
{
    int ptCount = ptList.size();
    if (ptCount < 1) return;

    area = 0;         // Accumulates area in the loop
    int j = ptCount - 1;  // The last vertex is the 'previous' one to the first

    for (int i = 0; i < ptCount; j = i++)
    {
        area += (ptList[j].easting + ptList[i].easting) * (ptList[j].northing - ptList[i].northing);
    }
    area = fabs(area / 2);
}

/*
//the non precalculated version
bool CBoundary::isPointInPolygon()
{
    bool result = false;
    int j = ptList.size() - 1;
    Vec2 testPoint (mf->pn->easting, mf->pn->northing);

    if (j < 10) return true;

    for (int i = 0; i < ptList.size(); j = i++)
    {
        //TODO: reformat this expression and make sure it's right
        result ^= ((ptList[i].northing > testPoint.northing) ^ (ptList[j].northing > testPoint.northing)) &&
            testPoint.easting < ((ptList[j].easting - ptList[i].easting) *
            (testPoint.northing - ptList[i].northing) / (ptList[j].northing - ptList[i].northing) + ptList[i].easting);
    }
    return result;
}
*/
