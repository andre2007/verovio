/////////////////////////////////////////////////////////////////////////////
// Name:        elementpart.cpp
// Author:      Laurent Pugin
// Created:     2017
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "elementpart.h"

//----------------------------------------------------------------------------

#include <assert.h>

//----------------------------------------------------------------------------

#include "btrem.h"
#include "chord.h"
#include "doc.h"
#include "functorparams.h"
#include "layer.h"
#include "note.h"
#include "smufl.h"
#include "staff.h"
#include "tuplet.h"
#include "vrv.h"

namespace vrv {

//----------------------------------------------------------------------------
// Dots
//----------------------------------------------------------------------------

Dots::Dots() : LayerElement(DOTS, "dots-"), AttAugmentDots()
{
    this->RegisterAttClass(ATT_AUGMENTDOTS);

    this->Reset();
}

Dots::~Dots() {}

void Dots::Reset()
{
    LayerElement::Reset();
    this->ResetAugmentDots();

    m_isAdjusted = false;
    m_flagShift = 0;
    m_dotLocsByStaff.clear();
}

std::set<int> Dots::GetDotLocsForStaff(Staff *staff) const
{
    if (m_dotLocsByStaff.find(staff) != m_dotLocsByStaff.end()) {
        return m_dotLocsByStaff.at(staff);
    }
    return {};
}

std::set<int> &Dots::ModifyDotLocsForStaff(Staff *staff)
{
    return m_dotLocsByStaff[staff];
}

//----------------------------------------------------------------------------
// Flag
//----------------------------------------------------------------------------

Flag::Flag() : LayerElement(FLAG, "flag-")
{

    this->Reset();
}

Flag::~Flag() {}

void Flag::Reset()
{
    LayerElement::Reset();

    m_drawingNbFlags = 0;
}

wchar_t Flag::GetFlagGlyph(data_STEMDIRECTION stemDir) const
{
    if (stemDir == STEMDIRECTION_up) {
        switch (m_drawingNbFlags) {
            case 1: return SMUFL_E240_flag8thUp;
            case 2: return SMUFL_E242_flag16thUp;
            case 3: return SMUFL_E244_flag32ndUp;
            case 4: return SMUFL_E246_flag64thUp;
            case 5: return SMUFL_E248_flag128thUp;
            case 6: return SMUFL_E24A_flag256thUp;
            case 7: return SMUFL_E24C_flag512thUp;
            case 8: return SMUFL_E24E_flag1024thUp;
            default: return 0;
        }
    }
    else {
        switch (m_drawingNbFlags) {
            case 1: return SMUFL_E241_flag8thDown;
            case 2: return SMUFL_E243_flag16thDown;
            case 3: return SMUFL_E245_flag32ndDown;
            case 4: return SMUFL_E247_flag64thDown;
            case 5: return SMUFL_E249_flag128thDown;
            case 6: return SMUFL_E24B_flag256thDown;
            case 7: return SMUFL_E24D_flag512thDown;
            case 8: return SMUFL_E24F_flag1024thDown;
            default: return 0;
        }
    }
}

Point Flag::GetStemUpSE(Doc *doc, int staffSize, bool graceSize, wchar_t &code) const
{
    code = this->GetFlagGlyph(STEMDIRECTION_up);

    return Point(0, doc->GetGlyphTop(code, staffSize, graceSize));
}

Point Flag::GetStemDownNW(Doc *doc, int staffSize, bool graceSize, wchar_t &code) const
{
    code = this->GetFlagGlyph(STEMDIRECTION_down);

    return Point(0, doc->GetGlyphBottom(code, staffSize, graceSize));
}

//----------------------------------------------------------------------------
// TupletBracket
//----------------------------------------------------------------------------

TupletBracket::TupletBracket() : LayerElement(TUPLET_BRACKET, "bracket-"), AttTupletVis()
{
    this->RegisterAttClass(ATT_TUPLETVIS);

    this->Reset();
}

TupletBracket::~TupletBracket() {}

void TupletBracket::Reset()
{
    LayerElement::Reset();
    this->ResetTupletVis();

    m_drawingXRelLeft = 0;
    m_drawingXRelRight = 0;
    m_alignedNum = NULL;
}

int TupletBracket::GetDrawingXLeft()
{
    Tuplet *tuplet = vrv_cast<Tuplet *>(this->GetFirstAncestor(TUPLET));
    assert(tuplet && tuplet->GetDrawingLeft());

    return tuplet->GetDrawingLeft()->GetDrawingX() + m_drawingXRelLeft;
}

int TupletBracket::GetDrawingXRight()
{
    Tuplet *tuplet = vrv_cast<Tuplet *>(this->GetFirstAncestor(TUPLET));
    assert(tuplet && tuplet->GetDrawingRight());

    return tuplet->GetDrawingRight()->GetDrawingX() + m_drawingXRelRight;
}

int TupletBracket::GetDrawingYLeft()
{
    Tuplet *tuplet = vrv_cast<Tuplet *>(this->GetFirstAncestor(TUPLET));
    assert(tuplet && tuplet->GetDrawingLeft());

    Beam *beam = tuplet->GetBracketAlignedBeam();
    if (beam) {
        // Calculate the y point aligning with the beam
        int xLeft = tuplet->GetDrawingLeft()->GetDrawingX() + m_drawingXRelLeft;
        return beam->m_beamSegment.GetStartingY()
            + beam->m_beamSegment.m_beamSlope * (xLeft - beam->m_beamSegment.GetStartingX()) + this->GetDrawingYRel();
    }
    else {
        return this->GetDrawingY();
    }
}

int TupletBracket::GetDrawingYRight()
{
    Tuplet *tuplet = vrv_cast<Tuplet *>(this->GetFirstAncestor(TUPLET));
    assert(tuplet && tuplet->GetDrawingRight());

    Beam *beam = tuplet->GetBracketAlignedBeam();
    if (beam) {
        // Calculate the y point aligning with the beam
        int xRight = tuplet->GetDrawingRight()->GetDrawingX() + m_drawingXRelRight;
        return beam->m_beamSegment.GetStartingY()
            + beam->m_beamSegment.m_beamSlope * (xRight - beam->m_beamSegment.GetStartingX()) + this->GetDrawingYRel();
    }
    else {
        return this->GetDrawingY();
    }
}

//----------------------------------------------------------------------------
// TupletNum
//----------------------------------------------------------------------------

TupletNum::TupletNum() : LayerElement(TUPLET_NUM, "num-"), AttNumberPlacement(), AttTupletVis()
{
    this->RegisterAttClass(ATT_NUMBERPLACEMENT);
    this->RegisterAttClass(ATT_TUPLETVIS);

    this->Reset();
}

TupletNum::~TupletNum() {}

void TupletNum::Reset()
{
    LayerElement::Reset();
    this->ResetNumberPlacement();
    this->ResetTupletVis();

    m_alignedBracket = NULL;
}

int TupletNum::GetDrawingYMid()
{
    if (m_alignedBracket) {
        int yLeft = m_alignedBracket->GetDrawingYLeft();
        int yRight = m_alignedBracket->GetDrawingYRight();
        return yLeft + ((yRight - yLeft) / 2);
    }
    else {
        return this->GetDrawingY();
    }
}

int TupletNum::GetDrawingXMid(Doc *doc)
{
    if (m_alignedBracket) {
        int xLeft = m_alignedBracket->GetDrawingXLeft();
        int xRight = m_alignedBracket->GetDrawingXRight();
        return xLeft + ((xRight - xLeft) / 2);
    }
    else {
        Tuplet *tuplet = vrv_cast<Tuplet *>(this->GetFirstAncestor(TUPLET));
        assert(tuplet && tuplet->GetDrawingLeft() && tuplet->GetDrawingRight());
        int xLeft = tuplet->GetDrawingLeft()->GetDrawingX();
        int xRight = tuplet->GetDrawingRight()->GetDrawingX();
        if (doc) {
            xRight += (tuplet->GetDrawingRight()->GetDrawingRadius(doc) * 2);
        }
        if (tuplet->GetNumAlignedBeam()) {
            Beam *beam = tuplet->GetNumAlignedBeam();
            switch (beam->m_drawingPlace) {
                case BEAMPLACE_above: xLeft += (tuplet->GetDrawingLeft()->GetDrawingRadius(doc)); break;
                case BEAMPLACE_below: xRight -= (tuplet->GetDrawingRight()->GetDrawingRadius(doc)); break;
                default: break;
            }
        }
        return xLeft + ((xRight - xLeft) / 2);
    }
}

void TupletNum::SetAlignedBracket(TupletBracket *alignedBracket)
{
    if (m_alignedBracket) m_alignedBracket->SetAlignedNum(NULL);
    m_alignedBracket = alignedBracket;
    if (m_alignedBracket) m_alignedBracket->SetAlignedNum(this);
}

//----------------------------------------------------------------------------
// Stem
//----------------------------------------------------------------------------

Stem::Stem() : LayerElement(STEM, "stem-"), AttGraced(), AttStems(), AttStemsCmn()
{
    this->RegisterAttClass(ATT_GRACED);
    this->RegisterAttClass(ATT_STEMS);
    this->RegisterAttClass(ATT_STEMSCMN);

    this->Reset();
}

Stem::~Stem() {}

void Stem::Reset()
{
    LayerElement::Reset();
    this->ResetGraced();
    this->ResetStems();
    this->ResetStemsCmn();

    m_drawingStemDir = STEMDIRECTION_NONE;
    m_drawingStemLen = 0;
    m_isVirtual = false;
}

bool Stem::IsSupportedChild(Object *child)
{
    if (child->Is(FLAG)) {
        assert(dynamic_cast<Flag *>(child));
    }
    else {
        return false;
    }
    return true;
}

int Stem::CompareToElementPosition(Doc *doc, LayerElement *otherElement, int margin)
{
    Staff *staff = this->GetAncestorStaff();

    // check if there is an overlap on the left or on the right and displace stem's parent correspondingly
    const int right = HorizontalLeftOverlap(otherElement, doc, margin, 0);
    const int left = HorizontalRightOverlap(otherElement, doc, margin, 0);
    if (!right || !left) return 0;

    int horizontalMargin = 2 * doc->GetDrawingStemWidth(staff->m_drawingStaffSize);
    Flag *currentFlag = NULL;
    currentFlag = vrv_cast<Flag *>(FindDescendantByType(FLAG, 1));
    if (currentFlag && currentFlag->m_drawingNbFlags) {
        wchar_t flagGlyph = currentFlag->GetFlagGlyph(STEMDIRECTION_down);
        const int flagWidth = doc->GetGlyphWidth(flagGlyph, staff->m_drawingStaffSize, this->GetDrawingCueSize());
        horizontalMargin += flagWidth;
    }

    if (right < left) {
        return right + horizontalMargin;
    }
    else {
        return (-horizontalMargin - left);
    }
}

void Stem::AdjustFlagPlacement(Doc *doc, Flag *flag, int staffSize, int verticalCenter, int duration)
{
    LayerElement *parent = vrv_cast<LayerElement *>(this->GetParent());
    if (!parent) return;

    const data_STEMDIRECTION stemDirection = this->GetDrawingStemDir();
    // For overlapping purposes we don't care for flags shorter than 16th since they grow in opposite direction
    wchar_t flagGlyph = SMUFL_E242_flag16thUp;
    if (duration < DURATION_16) flagGlyph = flag->GetFlagGlyph(stemDirection);
    const int glyphHeight = doc->GetGlyphHeight(flagGlyph, staffSize, this->GetDrawingCueSize());

    // Make sure that flags don't overlap with notehead. Upward flags cannot overlap with noteheads so check
    // only downward ones
    const int adjustmentStep = doc->GetDrawingUnit(staffSize);
    if (stemDirection == STEMDIRECTION_down) {
        const int noteheadMargin = this->GetDrawingStemLen() - (glyphHeight + parent->GetDrawingRadius(doc));
        if ((duration > DURATION_16) && (noteheadMargin < 0)) {
            int offset = 0;
            if (noteheadMargin % adjustmentStep < -adjustmentStep / 3 * 2) offset = adjustmentStep / 2;
            const int heightToAdjust = (noteheadMargin / adjustmentStep) * adjustmentStep - offset;
            this->SetDrawingStemLen(this->GetDrawingStemLen() - heightToAdjust);
            flag->SetDrawingYRel(-this->GetDrawingStemLen());
        }
    }

    Note *note = NULL;
    if (parent->Is(NOTE)) {
        note = vrv_cast<Note *>(parent);
    }
    else if (parent->Is(CHORD)) {
        note = vrv_cast<Chord *>(parent)->GetTopNote();
    }
    int ledgerAbove = 0;
    int ledgerBelow = 0;
    if (!note || !note->HasLedgerLines(ledgerAbove, ledgerBelow)) return;
    if (((stemDirection == STEMDIRECTION_up) && !ledgerBelow)
        || ((stemDirection == STEMDIRECTION_down) && !ledgerAbove))
        return;

    // Make sure that flags don't overlap with first (top or bottom) ledger line (effectively avoiding all ledgers)
    const int directionBias = (stemDirection == STEMDIRECTION_down) ? -1 : 1;
    const int position = this->GetDrawingY() - this->GetDrawingStemLen() - directionBias * glyphHeight;
    const int ledgerPosition = verticalCenter - 6 * directionBias * adjustmentStep;
    const int displacementMargin = (position - ledgerPosition) * directionBias;

    if (displacementMargin < 0) {
        int offset = 0;
        if ((stemDirection == STEMDIRECTION_down) && (displacementMargin % adjustmentStep > -adjustmentStep / 3)) {
            offset = adjustmentStep / 2;
        }
        const int heightToAdjust = (displacementMargin / adjustmentStep - 1) * adjustmentStep * directionBias - offset;
        this->SetDrawingStemLen(this->GetDrawingStemLen() + heightToAdjust);
        flag->SetDrawingYRel(-this->GetDrawingStemLen());
    }
}

//----------------------------------------------------------------------------
// Functors methods
//----------------------------------------------------------------------------

int Dots::ResetDrawing(FunctorParams *functorParams)
{
    // Call parent one too
    LayerElement::ResetDrawing(functorParams);

    m_dotLocsByStaff.clear();
    m_isAdjusted = false;

    return FUNCTOR_CONTINUE;
}

int Dots::ResetHorizontalAlignment(FunctorParams *functorParams)
{
    LayerElement::ResetHorizontalAlignment(functorParams);

    m_dotLocsByStaff.clear();

    return FUNCTOR_CONTINUE;
}

int Flag::ResetDrawing(FunctorParams *functorParams)
{
    // Call parent one too
    LayerElement::ResetDrawing(functorParams);

    m_drawingNbFlags = 0;

    return FUNCTOR_CONTINUE;
}

int TupletBracket::ResetHorizontalAlignment(FunctorParams *functorParams)
{
    // Call parent one too
    LayerElement::ResetHorizontalAlignment(functorParams);

    m_drawingXRelLeft = 0;
    m_drawingXRelRight = 0;
    m_alignedNum = NULL;

    return FUNCTOR_CONTINUE;
}

int TupletBracket::ResetVerticalAlignment(FunctorParams *functorParams)
{
    // Call parent one too
    LayerElement::ResetVerticalAlignment(functorParams);

    return FUNCTOR_CONTINUE;
}

int TupletNum::ResetHorizontalAlignment(FunctorParams *functorParams)
{
    // Call parent one too
    LayerElement::ResetHorizontalAlignment(functorParams);

    m_alignedBracket = NULL;

    return FUNCTOR_CONTINUE;
}

int TupletNum::ResetVerticalAlignment(FunctorParams *functorParams)
{
    // Call parent one too
    LayerElement::ResetVerticalAlignment(functorParams);

    return FUNCTOR_CONTINUE;
}

int Stem::CalcStem(FunctorParams *functorParams)
{
    CalcStemParams *params = vrv_params_cast<CalcStemParams *>(functorParams);
    assert(params);

    assert(params->m_staff);
    assert(params->m_layer);
    assert(params->m_interface);

    int staffSize = params->m_staff->m_drawingStaffSize;
    int stemShift = params->m_doc->GetDrawingStemWidth(staffSize) / 2;
    bool drawingCueSize = this->GetDrawingCueSize();

    // For notes longer than half notes the stem is always 0
    if (params->m_dur < DUR_2) {
        this->SetDrawingXRel(0);
        this->SetDrawingYRel(0);
        this->SetDrawingStemLen(0);
        return FUNCTOR_CONTINUE;
    }

    /************ Set the position, the length and adjust to the note head ************/

    const int unit = params->m_doc->GetDrawingUnit(staffSize);
    int baseStem = 0;
    // Use the given one if any
    if (this->HasStemLen()) {
        baseStem = this->GetStemLen() * -unit;
    }
    // Do not adjust the baseStem for stem sameas notes (its length is in m_chordStemLength)
    else if (!params->m_isStemSameasSecondary) {
        int thirdUnit = unit / 3;
        const data_STEMDIRECTION stemDir = params->m_interface->GetDrawingStemDir();
        baseStem = -(params->m_interface->CalcStemLenInThirdUnits(params->m_staff, stemDir) * thirdUnit);
        if (drawingCueSize) baseStem = params->m_doc->GetCueSize(baseStem);
    }
    // Even if a stem length is given we add the length of the chord content (however only if not 0)
    // Also, the given stem length is understood as being measured from the center of the note.
    // This means that it will be adjusted according to the note head (see below
    if (!params->m_staff || !this->HasStemLen() || (this->GetStemLen() != 0)) {
        Point p;
        if (this->GetDrawingStemDir() == STEMDIRECTION_up) {
            if (this->GetStemPos() == STEMPOSITION_left) {
                p = params->m_interface->GetStemDownNW(params->m_doc, staffSize, drawingCueSize);
                p.x += stemShift;
            }
            else {
                p = params->m_interface->GetStemUpSE(params->m_doc, staffSize, drawingCueSize);
                p.x -= stemShift;
            }
            const int stemShotening = (params->m_isStemSameasSecondary) ? 0 : p.y;
            this->SetDrawingStemLen(baseStem + params->m_chordStemLength + stemShotening);
        }
        else {
            if (this->GetStemPos() == STEMPOSITION_right) {
                p = params->m_interface->GetStemUpSE(params->m_doc, staffSize, drawingCueSize);
                p.x -= stemShift;
            }
            else {
                p = params->m_interface->GetStemDownNW(params->m_doc, staffSize, drawingCueSize);
                p.x += stemShift;
            }
            const int stemShotening = (params->m_isStemSameasSecondary) ? 0 : p.y;
            this->SetDrawingStemLen(-(baseStem + params->m_chordStemLength - stemShotening));
        }
        this->SetDrawingYRel(this->GetDrawingYRel() + p.y);
        this->SetDrawingXRel(p.x);
    }

    /************ Set flag and slashes (if necessary) and adjust the length ************/
    data_STEMMODIFIER stemMod = STEMMODIFIER_NONE;
    if (!params->m_isStemSameasSecondary) {
        BTrem *bTrem = vrv_cast<BTrem *>(this->GetFirstAncestor(BTREM));
        if (bTrem) {
            stemMod = bTrem->GetDrawingStemMod();
        }
        else if (this->HasStemMod() && (this->GetStemMod() < 8)) {
            stemMod = this->GetDrawingStemMod();
        }
    }

    int flagOffset = 0;
    Flag *flag = NULL;
    // There is never a flag with a duration longer than 8th notes
    if (params->m_dur > DUR_4) {
        flag = vrv_cast<Flag *>(this->GetFirst(FLAG));
        assert(flag);
        // There is never a flag with stem sameas notes
        if (params->m_isStemSameasSecondary) {
            flag->m_drawingNbFlags = 0;
        }
        else {
            flag->m_drawingNbFlags = params->m_dur - DUR_4;
            flagOffset = unit * (flag->m_drawingNbFlags - 1);
        }
    }

    const wchar_t code = this->StemModeToGlyph(stemMod);
    const int actualLength = this->GetDrawingStemLen() / unit * unit;
    const int diff = std::abs(actualLength) - params->m_doc->GetGlyphHeight(code, staffSize, false);
    // Adjust basic stem length to number of slashes
    if ((stemMod != STEMMODIFIER_NONE) && !this->HasStemLen() && (diff < unit * 2)) {
        int adjust = (2 * unit - diff) / unit * unit;
        if (stemMod == STEMMODIFIER_6slash) {
            adjust += params->m_doc->GetGlyphHeight(SMUFL_E220_tremolo1, staffSize, false);
        }
        if (this->GetDrawingStemDir() == STEMDIRECTION_up) {
            this->SetDrawingStemLen(this->GetDrawingStemLen() - adjust - flagOffset);
        }
        else {
            this->SetDrawingStemLen(this->GetDrawingStemLen() + adjust + flagOffset);
        }
    }

    // SMUFL flags cover some additional stem length from the 32th only
    if (flag) {
        flag->SetDrawingYRel(-this->GetDrawingStemLen());
    }

    // Do not adjust the length with stem sameas notes or if given in the encoding
    // however, the stem will be extend with the SMuFL extension from 32th - this can be improved
    if (params->m_isStemSameasSecondary || this->HasStemLen()) {
        if ((this->GetStemLen() == 0) && flag) flag->m_drawingNbFlags = 0;
        return FUNCTOR_CONTINUE;
    }
    if ((this->GetStemVisible() == BOOLEAN_false) && flag) {
        flag->m_drawingNbFlags = 0;
        return FUNCTOR_CONTINUE;
    }

    int flagHeight = 0;

    // SMUFL flags cover some additional stem length from the 32th only
    if (params->m_dur > DUR_16) {
        assert(flag);
        Point stemEnd;
        wchar_t flagCode = 0;
        if (this->GetDrawingStemDir() == STEMDIRECTION_up) {
            stemEnd = flag->GetStemUpSE(params->m_doc, staffSize, drawingCueSize, flagCode);
        }
        else {
            stemEnd = flag->GetStemDownNW(params->m_doc, staffSize, drawingCueSize, flagCode);
        }
        // Trick for shortening the stem with DUR_8
        flagHeight = stemEnd.y;
    }

    int endY = this->GetDrawingY() - this->GetDrawingStemLen() + flagHeight;
    bool adjust = false;
    if ((this->GetDrawingStemDir() == STEMDIRECTION_up) && (endY < params->m_verticalCenter)) {
        adjust = true;
    }
    else if ((this->GetDrawingStemDir() == STEMDIRECTION_down) && (endY > params->m_verticalCenter)) {
        adjust = true;
    }

    // Do not adjust the length of grace notes - this is debatable and should probably become as styling option
    // However we still want flags from grace notes not to overlap with ledger lines
    if (adjust && !params->m_isGraceNote) {
        this->SetDrawingStemLen(this->GetDrawingStemLen() + (endY - params->m_verticalCenter));
        if (flag) flag->SetDrawingYRel(-this->GetDrawingStemLen());
    }

    if (flag) AdjustFlagPlacement(params->m_doc, flag, staffSize, params->m_verticalCenter, params->m_dur);

    return FUNCTOR_CONTINUE;
}

int Stem::ResetDrawing(FunctorParams *functorParams)
{
    // Call parent one too
    LayerElement::ResetDrawing(functorParams);

    m_drawingStemDir = STEMDIRECTION_NONE;
    m_drawingStemLen = 0;

    return FUNCTOR_CONTINUE;
}

} // namespace vrv
