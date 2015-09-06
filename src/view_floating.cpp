/////////////////////////////////////////////////////////////////////////////
// Name:        view_floating.cpp
// Author:      Laurent Pugin
// Created:     2015
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include "view.h"

//----------------------------------------------------------------------------

#include <assert.h>
#include <iostream>
#include <math.h>

//----------------------------------------------------------------------------

//#include "accid.h"
#include "att_comparison.h"
//#include "beam.h"
//#include "chord.h"
//#include "custos.h"
#include "devicecontext.h"
#include "doc.h"
//#include "dot.h"
#include "floatingelement.h"
//#include "keysig.h"
#include "layer.h"
#include "layerelement.h"
#include "measure.h"
//#include "mensur.h"
//#include "metersig.h"
//#include "mrest.h"
//#include "multirest.h"
#include "note.h"
//#include "rest.h"
#include "slur.h"
//#include "space.h"
//#include "smufl.h"
#include "staff.h"
#include "style.h"
#include "syl.h"
#include "system.h"
#include "tie.h"
#include "timeinterface.h"
//#include "tuplet.h"
//#include "verse.h"
#include "vrv.h"

namespace vrv {

//----------------------------------------------------------------------------
// View - FloatingElement
//----------------------------------------------------------------------------
    
void View::DrawFloatingElement( DeviceContext *dc, FloatingElement *element, Measure *measure, System *system)
{
    assert( dc );
    assert( system );
    assert( measure );
    assert( element );
    
    if (element->HasInterface(INTERFACE_TIME_SPANNING)) {
        // creating placeholder
        dc->StartGraphic( element, "", element->GetUuid() );
        dc->EndGraphic( element, this);
        system->AddToDrawingList(element);
    }
}

void View::DrawTimeSpanningElement( DeviceContext *dc, DocObject *element, System *system )
{
    assert( dc );
    assert( element );
    assert( system );
    
    TimeSpanningInterface *interface = dynamic_cast<TimeSpanningInterface*>(element);
    assert( interface );
    
    if ( !interface->HasStartAndEnd() ) return;
    
    // Get the parent system of the first and last note
    System *parentSystem1 = dynamic_cast<System*>( interface->GetStart()->GetFirstParent( SYSTEM )  );
    System *parentSystem2 = dynamic_cast<System*>( interface->GetEnd()->GetFirstParent( SYSTEM )  );
    
    int x1, x2;
    Staff *staff = NULL;
    DocObject *graphic = NULL;
    char spanningType = SPANNING_START_END;
    
    // The both correspond to the current system, which means no system break in-between (simple case)
    if (( system == parentSystem1 ) && ( system == parentSystem2 )) {
        // Get the parent staff for calculating the y position
        staff = dynamic_cast<Staff*>( interface->GetStart()->GetFirstParent( STAFF ) );
        if ( !Check( staff ) ) return;
        
        x1 = interface->GetStart()->GetDrawingX();
        x2 = interface->GetEnd()->GetDrawingX();
        graphic = element;
    }
    // Only the first parent is the same, this means that the element is "open" at the end of the system
    else if ( system == parentSystem1 ) {
        // We need the last measure of the system for x2
        Measure *last = dynamic_cast<Measure*>( system->FindChildByType( MEASURE, 1, BACKWARD ) );
        if ( !Check( last ) ) return;
        staff = dynamic_cast<Staff*>( interface->GetStart()->GetFirstParent( STAFF ) );
        if ( !Check( staff ) ) return;
        
        x1 = interface->GetStart()->GetDrawingX();
        x2 = last->GetDrawingX() + last->GetRightBarlineX();
        graphic = element;
        spanningType = SPANNING_START;
    }
    // We are in the system of the last note - draw the element from the beginning of the system
    else if ( system == parentSystem2 ) {
        // We need the first measure of the system for x1
        Measure *first = dynamic_cast<Measure*>( system->FindChildByType( MEASURE, 1, FORWARD ) );
        if ( !Check( first ) ) return;
        // Get the staff of the first note - however, not the staff we need
        Staff *lastStaff = dynamic_cast<Staff*>( interface->GetEnd()->GetFirstParent( STAFF ) );
        if ( !Check( lastStaff ) ) return;
        // We need the first staff from the current system, i.e., the first measure.
        AttCommonNComparison comparison( STAFF, lastStaff->GetN() );
        staff = dynamic_cast<Staff*>(system->FindChildByAttComparison(&comparison, 2));
        if (!staff ) {
            LogDebug("Could not get staff (%d) while drawing staffGrp - View::DrawSylConnector", lastStaff->GetN() );
            return;
        }
        // Also try to get a first note - we should change this once we have a x position in measure that
        // takes into account the scoreDef
        Note *firstNote = dynamic_cast<Note*>( staff->FindChildByType( NOTE ) );
        
        x1 = firstNote ? firstNote->GetDrawingX() - 2 * m_doc->GetDrawingDoubleUnit(staff->m_drawingStaffSize) : first->GetDrawingX();
        x2 = interface->GetEnd()->GetDrawingX();
        spanningType = SPANNING_END;
    }
    // Rare case where neither the first note and the last note are in the current system - draw the connector throughout the system
    else {
        // We need the first measure of the system for x1
        Measure *first = dynamic_cast<Measure*>( system->FindChildByType( MEASURE, 1, FORWARD ) );
        if ( !Check( first ) ) return;
        // Also try to get a first note - we should change this once we have a x position in measure that
        // takes into account the scoreDef
        Note *firstNote = dynamic_cast<Note*>( first->FindChildByType( NOTE ) );
        // We need the last measure of the system for x2
        Measure *last = dynamic_cast<Measure*>( system->FindChildByType( MEASURE, 1, BACKWARD ) );
        if ( !Check( last ) ) return;
        // Get the staff of the first note - however, not the staff we need
        Staff *firstStaff = dynamic_cast<Staff*>( interface->GetStart()->GetFirstParent( STAFF ) );
        if ( !Check( firstStaff ) ) return;
        
        // We need the staff from the current system, i.e., the first measure.
        AttCommonNComparison comparison( STAFF, firstStaff->GetN() );
        staff = dynamic_cast<Staff*>(first->FindChildByAttComparison(&comparison, 1));
        if (!staff ) {
            LogDebug("Could not get staff (%d) while drawing staffGrp - View::DrawSylConnector", firstStaff->GetN() );
            return;
        }
        
        x1 = firstNote ? firstNote->GetDrawingX() - 2 * m_doc->GetDrawingDoubleUnit(staff->m_drawingStaffSize) : first->GetDrawingX();
        x2 = last->GetDrawingX() + last->GetRightBarlineX();
        spanningType = SPANNING_MIDDLE;
    }
    
    if (element->Is() == SLUR) {
        // cast to Slur check in DrawTieOrSlur
        DrawSlur(dc, dynamic_cast<Slur*>(element), x1, x2, staff, spanningType, graphic);
    }
    else if (element->Is() == SYL) {
        // cast to Syl check in DrawSylConnector
        DrawSylConnector(dc, dynamic_cast<Syl*>(element), x1, x2, staff, spanningType, graphic);
    }
    else if (element->Is() == TIE) {
        // cast to Slur check in DrawTieOrSlur
        DrawTie(dc, dynamic_cast<Tie*>(element), x1, x2, staff, spanningType, graphic);
    }
    
}

void View::DrawSlur( DeviceContext *dc, Slur *slur, int x1, int x2, Staff *staff,
                         char spanningType, DocObject *graphic )
{
    assert( dc );
    assert( slur );
    assert( staff );
    
    Note *note1 = NULL;
    Note *note2 = NULL;
    
    bool up = true;
    data_STEMDIRECTION noteStemDir = STEMDIRECTION_NONE;
    int y1, y2;
    
    /************** parent layers **************/
    
    note1 = dynamic_cast<Note*>(slur->GetStart());
    note2 = dynamic_cast<Note*>(slur->GetEnd());
    
    if ( !note1 || !note2 ) {
        // no note, obviously nothing to do...
        return;
    }
    
    Chord *chordParent1 = note1->IsChordTone();
    
    Layer* layer1 = dynamic_cast<Layer*>(note1->GetFirstParent( LAYER ) );
    Layer* layer2 = dynamic_cast<Layer*>(note2->GetFirstParent( LAYER ) );
    assert( layer1 && layer2 );
    
    if ( layer1->GetN() != layer2->GetN() ) {
        LogWarning("Ties between different layers may not be fully supported.");
    }
    
    /************** x positions **************/
    
    // the normal case
    if ( spanningType == SPANNING_START_END ) {
        y1 = note1->GetDrawingY();
        y2 = note2->GetDrawingY();
        noteStemDir = note1->GetDrawingStemDir();
    }
    // This is the case when the tie is split over two system of two pages.
    // In this case, we are now drawing its beginning to the end of the measure (i.e., the last aligner)
    else if ( spanningType == SPANNING_START ) {
        y1 = note1->GetDrawingY();
        y2 = y1;
        noteStemDir = note1->GetDrawingStemDir();
    }
    // Now this is the case when the tie is split but we are drawing the end of it
    else if ( spanningType == SPANNING_END ) {
        y1 = note2->GetDrawingY();
        y2 = y1;
        noteStemDir = note2->GetDrawingStemDir();
    }
    // Finally
    else {
        LogDebug("Tie across an entire system is not supported");
        return;
    }
    
    /************** direction **************/
    
    // first should be the tie @curvedir
    if (slur->HasCurvedir()) {
        up = (slur->GetCurvedir() == CURVEDIR_above) ? true : false;
    }
    // then layer direction trumps note direction
    else if (layer1 && layer1->GetDrawingStemDir() != STEMDIRECTION_NONE){
        up = layer1->GetDrawingStemDir() == STEMDIRECTION_up ? true : false;
    }
    //  the look if in a chord
    else if (note1->IsChordTone()) {
        if (note1->PositionInChord() < 0) up = false;
        else if (note1->PositionInChord() > 0) up = true;
        // away from the stem if odd number (center note)
        else up = (noteStemDir != STEMDIRECTION_up);
    }
    else if (noteStemDir == STEMDIRECTION_up) {
        up = false;
    }
    else if (noteStemDir == STEMDIRECTION_NONE) {
        // no information from the note stem directions, look at the position in the notes
        int center = staff->GetDrawingY() - m_doc->GetDrawingDoubleUnit(staff->m_drawingStaffSize) * 2;
        up = (y1 > center) ? true : false;
    }

    /************** y position **************/
    
    if (up) {
        y1 += 2 * m_doc->GetDrawingUnit(staff->m_drawingStaffSize);
        y2 += 2 * m_doc->GetDrawingUnit(staff->m_drawingStaffSize);
    }
    else {
        y1 -= 2 * m_doc->GetDrawingUnit(staff->m_drawingStaffSize);
        y2 -= 2 * m_doc->GetDrawingUnit(staff->m_drawingStaffSize);
    }
    
    /************** bezier points **************/
    
    float slurAngle = atan2(y2 - y1, x2 - x1);
    Point center = Point(x1, y1);
    Point rotatedP2 = View::CalcPositionAfterRotation(Point(x2, y2), -slurAngle, center);
    //LogDebug("P1 %d %d, P2 %d %d, Angle %f, Pres %d %d", x1, y1, x2, y2, slurAnge, rotadedP2.x, rotatedP2.y);
    
    
    
    // the 'height' of the bezier
    int height;
    if (slur->HasBulge()){
        height = m_doc->GetDrawingUnit(staff->m_drawingStaffSize) * slur->GetBulge();
    }
    else {
        height = m_doc->GetDrawingUnit(staff->m_drawingStaffSize);
        // if the space between the to points is more than two staff height, increase the height
        if (x2 - x1 > 2 * m_doc->GetDrawingStaffSize(staff->m_drawingStaffSize)) {
            height +=  m_doc->GetDrawingUnit(staff->m_drawingStaffSize);
        }
    }
    int thickness =  m_doc->GetDrawingUnit(staff->m_drawingStaffSize) * m_doc->GetSlurThickness() / DEFINITON_FACTOR ;
    
    // control points
    Point rotatecC1, rotatedC2;
    
    // the height of the control points
    height = height * 4 / 3;
    
    rotatecC1.x = x1 + (rotatedP2.x - x1) / 4; // point at 1/4
    rotatedC2.x = x1 + (rotatedP2.x - x1) / 4 * 3; // point at 3/4
    
    if (up) {
        rotatecC1.y = y1 + height;
        rotatedC2.y = rotatedP2.y + height;
    } else {
        rotatecC1.y = y1 - height;
        rotatedC2.y = rotatedP2.y - height;
    }
    
    Point actualP2 = View::CalcPositionAfterRotation(rotatedP2, slurAngle, center);
    Point actualC1 = View::CalcPositionAfterRotation(rotatecC1, slurAngle, center);
    Point actualC2 = View::CalcPositionAfterRotation(rotatedC2, slurAngle, center);
    
    if ( graphic ) dc->ResumeGraphic(graphic, graphic->GetUuid());
    else dc->StartGraphic(slur, "spanning-slur", "");
    dc->DeactivateGraphic();
    DrawThickBezierCurve(dc, center, actualP2, actualC1, actualC2, thickness, staff->m_drawingStaffSize );
    dc->ReactivateGraphic();
    
    if ( graphic ) dc->EndResumedGraphic(graphic, this);
    else dc->EndGraphic(slur, this);
}
    
void View::DrawTie( DeviceContext *dc, Tie *tie, int x1, int x2, Staff *staff,
                         char spanningType, DocObject *graphic )
{
    assert( dc );
    assert( tie );
    assert( staff );
    
    Note *note1 = NULL;
    Note *note2 = NULL;
    
    bool up = true;
    data_STEMDIRECTION noteStemDir = STEMDIRECTION_NONE;
    int y1, y2;
    
    /************** parent layers **************/
    
    note1 = dynamic_cast<Note*>(tie->GetStart());
    note2 = dynamic_cast<Note*>(tie->GetEnd());
    
    if ( !note1 || !note2 ) {
        // no note, obviously nothing to do...
        return;
    }
    
    Chord *chordParent1 = note1->IsChordTone();
    
    Layer* layer1 = dynamic_cast<Layer*>(note1->GetFirstParent( LAYER ) );
    Layer* layer2 = dynamic_cast<Layer*>(note2->GetFirstParent( LAYER ) );
    assert( layer1 && layer2 );
    
    if ( layer1->GetN() != layer2->GetN() ) {
        LogWarning("Ties between different layers may not be fully supported.");
    }
    
    /************** x positions **************/
    
    bool isShortTie = false;
    // shortTie correction cannot be applied for chords
    if (!chordParent1 && (x2 - x1 < 3 * m_doc->GetDrawingDoubleUnit(staff->m_drawingStaffSize))) isShortTie = true;
    
    // the normal case
    if ( spanningType == SPANNING_START_END ) {
        y1 = note1->GetDrawingY();
        y2 = note2->GetDrawingY();
        if (!isShortTie) {
            x1 += m_doc->GetDrawingUnit(staff->m_drawingStaffSize) * 3 / 2;
            x2 -= m_doc->GetDrawingUnit(staff->m_drawingStaffSize) * 3 / 2;
            if (note1->HasDots()) {
                x1 += m_doc->GetDrawingDoubleUnit(staff->m_drawingStaffSize) * note1->GetDots();
            }
            else if (chordParent1 && chordParent1->HasDots()) {
                x1 += m_doc->GetDrawingDoubleUnit(staff->m_drawingStaffSize) * chordParent1->GetDots();
            }
        }
        noteStemDir = note1->GetDrawingStemDir();
    }
    // This is the case when the tie is split over two system of two pages.
    // In this case, we are now drawing its beginning to the end of the measure (i.e., the last aligner)
    else if ( spanningType == SPANNING_START ) {
        y1 = note1->GetDrawingY();
        y2 = y1;
        if (!isShortTie) {
            x1 += m_doc->GetDrawingUnit(staff->m_drawingStaffSize) * 3 / 2;
        }
        noteStemDir = note1->GetDrawingStemDir();
    }
    // Now this is the case when the tie is split but we are drawing the end of it
    else if ( spanningType == SPANNING_END ) {
        y1 = note2->GetDrawingY();
        y2 = y1;
        if (!isShortTie) {
            x2 -= m_doc->GetDrawingUnit(staff->m_drawingStaffSize) * 3 / 2;
        }
        noteStemDir = note2->GetDrawingStemDir();
    }
    // Finally
    else {
        LogDebug("Tie across an entire system is not supported");
        return;
    }
    
    /************** direction **************/
    
    // first should be the tie @curvedir
    if (tie->HasCurvedir()) {
        up = (tie->GetCurvedir() == CURVEDIR_above) ? true : false;
    }
    // then layer direction trumps note direction
    else if (layer1 && layer1->GetDrawingStemDir() != STEMDIRECTION_NONE){
        up = layer1->GetDrawingStemDir() == STEMDIRECTION_up ? true : false;
    }
    //  the look if in a chord
    else if (note1->IsChordTone()) {
        if (note1->PositionInChord() < 0) up = false;
        else if (note1->PositionInChord() > 0) up = true;
        // away from the stem if odd number (center note)
        else up = (noteStemDir != STEMDIRECTION_up);
    }
    else if (noteStemDir == STEMDIRECTION_up) {
        up = false;
    }
    else if (noteStemDir == STEMDIRECTION_NONE) {
        // no information from the note stem directions, look at the position in the notes
        int center = staff->GetDrawingY() - m_doc->GetDrawingDoubleUnit(staff->m_drawingStaffSize) * 2;
        up = (y1 > center) ? true : false;
    }
    
    /************** y position **************/

    if (up) {
        y1 += m_doc->GetDrawingUnit(staff->m_drawingStaffSize) / 2;
        y2 += m_doc->GetDrawingUnit(staff->m_drawingStaffSize) / 2;
        if (isShortTie) {
            y1 += m_doc->GetDrawingUnit(staff->m_drawingStaffSize);
            y2 += m_doc->GetDrawingUnit(staff->m_drawingStaffSize);
        }
    }
    else {
        y1 -= m_doc->GetDrawingUnit(staff->m_drawingStaffSize) / 2;
        y2 -= m_doc->GetDrawingUnit(staff->m_drawingStaffSize) / 2;
        if (isShortTie) {
            y1 -= m_doc->GetDrawingUnit(staff->m_drawingStaffSize);
            y2 -= m_doc->GetDrawingUnit(staff->m_drawingStaffSize);
        }
        
    }
    
    /************** bezier points **************/
    
    // the 'height' of the bezier
    int height;
    if (tie->HasBulge()){
        height = m_doc->GetDrawingUnit(staff->m_drawingStaffSize) * tie->GetBulge();
    }
    else {
        height = m_doc->GetDrawingUnit(staff->m_drawingStaffSize);
        // if the space between the to points is more than two staff height, increase the height
        if (x2 - x1 > 2 * m_doc->GetDrawingStaffSize(staff->m_drawingStaffSize)) {
            height +=  m_doc->GetDrawingUnit(staff->m_drawingStaffSize);
        }
    }
    int thickness =  m_doc->GetDrawingUnit(staff->m_drawingStaffSize) * m_doc->GetTieThickness() / DEFINITON_FACTOR ;
    
    // control points
    Point c1, c2;
    
    // the height of the control points
    height = height * 4 / 3;
    
    c1.x = x1 + (x2 - x1) / 4; // point at 1/4
    c2.x = x1 + (x2 - x1) / 4 * 3; // point at 3/4
    
    if (up) {
        c1.y = y1 + height;
        c2.y = y2 + height;
    } else {
        c1.y = y1 - height;
        c2.y = y2 - height;
    }
    
    if ( graphic ) dc->ResumeGraphic(graphic, graphic->GetUuid());
    else dc->StartGraphic(tie, "spanning-tie", "");
    dc->DeactivateGraphic();
    DrawThickBezierCurve(dc, Point(x1, y1), Point(x2, y2), c1, c2, thickness, staff->m_drawingStaffSize );
    dc->ReactivateGraphic();
    if ( graphic ) dc->EndResumedGraphic(graphic, this);
    else dc->EndGraphic(tie, this);
}

void View::DrawSylConnector( DeviceContext *dc, Syl *syl, int x1, int x2, Staff *staff, char spanningType, DocObject *graphic )
{
    assert( syl );
    assert( syl->GetStart() && syl->GetEnd() );
    if ( !syl->GetStart() || !syl->GetEnd() ) return;
    
    int y = GetSylY(syl, staff);
    int w, h;
    
    // The both correspond to the current system, which means no system break in-between (simple case)
    if ( spanningType ==  SPANNING_START_END ) {
        dc->SetFont( m_doc->GetDrawingLyricFont( staff->m_drawingStaffSize ) );
        dc->GetTextExtent(syl->GetText(), &w, &h);
        dc->ResetFont();
        // x position of the syl is two units back
        x1 += w - m_doc->GetDrawingUnit(staff->m_drawingStaffSize) * 2;
    }
    // Only the first parent is the same, this means that the syl is "open" at the end of the system
    else  if ( spanningType ==  SPANNING_START) {
        dc->SetFont( m_doc->GetDrawingLyricFont( staff->m_drawingStaffSize ) );
        dc->GetTextExtent(syl->GetText(), &w, &h);
        dc->ResetFont();
        // idem
        x1 += w - m_doc->GetDrawingUnit(staff->m_drawingStaffSize) * 2;
        
    }
    // We are in the system of the last note - draw the connector from the beginning of the system
    else if ( spanningType ==  SPANNING_END ) {
        // nothing to adjust
    }
    // Rare case where neither the first note and the last note are in the current system - draw the connector throughout the system
    else {
        // nothing to adjust
    }
    
    if ( graphic ) dc->ResumeGraphic(graphic, graphic->GetUuid());
    else dc->StartGraphic(syl, "spanning-connector", "");
    dc->DeactivateGraphic();
    DrawSylConnectorLines( dc, x1, x2, y, syl, staff);
    dc->ReactivateGraphic();
    if ( graphic ) dc->EndResumedGraphic(graphic, this);
    else dc->EndGraphic(syl, this);
    
}

void View::DrawSylConnectorLines( DeviceContext *dc, int x1, int x2, int y, Syl *syl, Staff *staff )
{
    if (syl->GetCon() == CON_d) {
        
        y += m_doc->GetDrawingUnit(staff->m_drawingStaffSize) * 2 / 3;
        // x position of the syl is two units back
        x2 -= 2 * (int)m_doc->GetDrawingUnit(staff->m_drawingStaffSize);
        
        //if ( x1 > x2 ) {
        //    DrawFullRectangle(dc, x1, y + 2* m_doc->GetDrawingBarLineWidth(staff->m_drawingStaffSize), x2, y + 3 * m_doc->GetDrawingBarLineWidth(staff->m_drawingStaffSize));
        //    LogDebug("x1 > x2 (%d %d)", x1, x2 );
        //}
        
        // the length of the dash and the space between them - can be made a parameter
        int dashLength = m_doc->GetDrawingUnit(staff->m_drawingStaffSize) * 4 / 3;
        int dashSpace = m_doc->GetDrawingStaffSize(staff->m_drawingStaffSize) * 5 / 3;
        int halfDashLength = dashLength / 2;
        
        int dist = x2 - x1;
        int nbDashes = dist / dashSpace;
        
        int margin = dist / 2;
        // at least one dash
        if (nbDashes < 2) {
            nbDashes = 1;
        }
        else {
            margin = (dist - ((nbDashes - 1) * dashSpace)) / 2;
        }
        margin -= dashLength / 2;
        int i, x;
        for (i = 0; i < nbDashes; i++) {
            x = x1 + margin + (i *  dashSpace);
            DrawFullRectangle(dc, x - halfDashLength, y, x + halfDashLength, y + m_doc->GetDrawingBarLineWidth(staff->m_drawingStaffSize));
        }
        
    }
    else if (syl->GetCon() == CON_u) {
        x1 += (int)m_doc->GetDrawingUnit(staff->m_drawingStaffSize) / 2;
        DrawFullRectangle(dc, x1, y, x2, y + m_doc->GetDrawingBarLineWidth(staff->m_drawingStaffSize));
    }
    
}

} // namespace vrv    
    
