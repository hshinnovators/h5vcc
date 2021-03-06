/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "Simplify.h"

namespace Op {

#define INCLUDED_BY_SHAPE_OPS 1

#include "Simplify.cpp"

// FIXME: this and find chase should be merge together, along with
// other code that walks winding in angles
// OPTIMIZATION: Probably, the walked winding should be rolled into the angle structure
// so it isn't duplicated by walkers like this one
static Segment* findChaseOp(SkTDArray<Span*>& chase, int& nextStart, int& nextEnd) {
    while (chase.count()) {
        Span* span;
        chase.pop(&span);
        const Span& backPtr = span->fOther->span(span->fOtherIndex);
        Segment* segment = backPtr.fOther;
        nextStart = backPtr.fOtherIndex;
        SkTDArray<Angle> angles;
        int done = 0;
        if (segment->activeAngle(nextStart, done, angles)) {
            Angle* last = angles.end() - 1;
            nextStart = last->start();
            nextEnd = last->end();
   #if TRY_ROTATE
            *chase.insert(0) = span;
   #else
            *chase.append() = span;
   #endif
            return last->segment();
        }
        if (done == angles.count()) {
            continue;
        }
        SkTDArray<Angle*> sorted;
        bool sortable = Segment::SortAngles(angles, sorted);
        int angleCount = sorted.count();
#if DEBUG_SORT
        sorted[0]->segment()->debugShowSort(__FUNCTION__, sorted, 0);
#endif
        if (!sortable) {
            continue;
        }
        // find first angle, initialize winding to computed fWindSum
        int firstIndex = -1;
        const Angle* angle;
        do {
            angle = sorted[++firstIndex];
            segment = angle->segment();
        } while (segment->windSum(angle) == SK_MinS32);
    #if DEBUG_SORT
        segment->debugShowSort(__FUNCTION__, sorted, firstIndex);
    #endif
        int sumMiWinding = segment->updateWindingReverse(angle);
        int sumSuWinding = segment->updateOppWindingReverse(angle);
        if (segment->operand()) {
            SkTSwap<int>(sumMiWinding, sumSuWinding);
        }
        int nextIndex = firstIndex + 1;
        int lastIndex = firstIndex != 0 ? firstIndex : angleCount;
        Segment* first = NULL;
        do {
            SkASSERT(nextIndex != firstIndex);
            if (nextIndex == angleCount) {
                nextIndex = 0;
            }
            int maxWinding, sumWinding, oppMaxWinding, oppSumWinding;
            angle = sorted[nextIndex];
            segment = angle->segment();
            int start = angle->start();
            int end = angle->end();
            segment->setUpWindings(start, end, sumMiWinding, sumSuWinding,
                    maxWinding, sumWinding, oppMaxWinding, oppSumWinding);
            if (!segment->done(angle)) {
                if (!first) {
                    first = segment;
                    nextStart = start;
                    nextEnd = end;
                }
                (void) segment->markAngle(maxWinding, sumWinding, oppMaxWinding,
                    oppSumWinding, true, angle);
            }
        } while (++nextIndex != lastIndex);
        if (first) {
       #if TRY_ROTATE
            *chase.insert(0) = span;
       #else
            *chase.append() = span;
       #endif
            return first;
        }
    }
    return NULL;
}

/*
static bool windingIsActive(int winding, int oppWinding, int spanWinding, int oppSpanWinding,
        bool windingIsOp, ShapeOp op) {
    bool active = windingIsActive(winding, spanWinding);
    if (!active) {
        return false;
    }
    if (oppSpanWinding && windingIsActive(oppWinding, oppSpanWinding)) {
        switch (op) {
            case kIntersect_Op:
            case kUnion_Op:
                return true;
            case kDifference_Op: {
                int absSpan = abs(spanWinding);
                int absOpp = abs(oppSpanWinding);
                return windingIsOp ? absSpan < absOpp : absSpan > absOpp;
            }
            case kXor_Op:
                return spanWinding != oppSpanWinding;
            default:
                SkASSERT(0);
        }
    }
    bool opActive = oppWinding != 0;
    return gOpLookup[op][opActive][windingIsOp];
}
*/

static Segment* findSortableTopNew(SkTDArray<Contour*>& contourList, bool& firstContour, int& index,
        int& endIndex, SkPoint& topLeft, bool& unsortable) {
    Segment* current;
    bool allowTies = true;
    bool first = true;
    do {
        current = findSortableTop(contourList, index, endIndex, topLeft, unsortable, allowTies,
                true);
        if (!current) {
            if (first) {
                return NULL;
            }
            break;
        }
        first = false;
        if (firstContour) {
            current->initWinding(index, endIndex, 0, 0);
            firstContour = false;
            return current;
        }
        int minIndex = SkMin32(index, endIndex);
        int sumWinding = current->windSum(minIndex);
        if (sumWinding == SK_MinS32) {
            sumWinding = current->computeSum(index, endIndex, true);
            if (sumWinding != SK_MinS32) {
                return current;
            }
        }
        allowTies = false;
        int contourWinding = innerContourCheck(contourList, current, index, endIndex, false);
        if (contourWinding == SK_MinS32) {
            continue;
        }
        int oppContourWinding = innerContourCheck(contourList, current, index, endIndex, true);
        if (oppContourWinding == SK_MinS32) {
            continue;
        }
        current->initWinding(index, endIndex, contourWinding, oppContourWinding);
        return current;
    } while (true);
    // the simple upward projection of the unresolved points hit unsortable angles
    // shoot rays at right angles to the segment to find its winding, ignoring angle cases
    SkASSERT(0); // steal code from findSortableTopOld and put it here
    return current;
}

static bool bridgeOp(SkTDArray<Contour*>& contourList, const ShapeOp op,
        const int xorMask, const int xorOpMask, PathWrapper& simple) {
    bool firstContour = true;
    bool unsortable = false;
    bool topUnsortable = false;
    bool firstRetry = false;
    bool closable = true;
    SkPoint topLeft = {SK_ScalarMin, SK_ScalarMin};
    do {
        int index, endIndex;
        Segment* current = findSortableTopNew(contourList, firstContour, index, endIndex, topLeft,
                topUnsortable);
        if (!current) {
            if (topUnsortable) {
                topUnsortable = false;
                SkASSERT(!firstRetry);
                firstRetry = true;
                topLeft.fX = topLeft.fY = SK_ScalarMin;
                continue;
            }
            break;
        }
        SkTDArray<Span*> chaseArray;
        do {
            if (current->activeOp(index, endIndex, xorMask, xorOpMask, op)) {
                bool active = true;
                do {
            #if DEBUG_ACTIVE_SPANS
                    if (!unsortable && current->done()) {
                        debugShowActiveSpans(contourList);
                    }
            #endif
                    SkASSERT(unsortable || !current->done());
                    int nextStart = index;
                    int nextEnd = endIndex;
                    Segment* next = current->findNextOp(chaseArray, nextStart, nextEnd,
                            unsortable, op, xorMask, xorOpMask);
                    if (!next) {
                        SkASSERT(!unsortable);
                        if (!unsortable && simple.hasMove()
                                && current->verb() != SkPath::kLine_Verb
                                && !simple.isClosed()) {
                            current->addCurveTo(index, endIndex, simple, true);
                            SkASSERT(simple.isClosed());
                        }
                        active = false;
                        break;
                    }
                    current->addCurveTo(index, endIndex, simple, true);
                    current = next;
                    index = nextStart;
                    endIndex = nextEnd;
                } while (!simple.isClosed() && ((!unsortable) || !current->done()));
                if (active && !simple.isClosed()) {
                    SkASSERT(unsortable);
                    int min = SkMin32(index, endIndex);
                    if (!current->done(min)) {
                        current->addCurveTo(index, endIndex, simple, true);
                        current->markDoneBinary(min);
                    }
                    closable = false;
                }
                simple.close();
            } else {
                Span* last = current->markAndChaseDoneBinary(index, endIndex);
                if (last) {
                    *chaseArray.append() = last;
                }
            }
            current = findChaseOp(chaseArray, index, endIndex);
        #if DEBUG_ACTIVE_SPANS
            debugShowActiveSpans(contourList);
        #endif
            if (!current) {
                break;
            }
        } while (true);
    } while (true);
    return closable;
}

} // end of Op namespace


void operate(const SkPath& one, const SkPath& two, ShapeOp op, SkPath& result) {
    result.reset();
    result.setFillType(SkPath::kEvenOdd_FillType);
    // turn path into list of segments
    SkTArray<Op::Contour> contours;
    // FIXME: add self-intersecting cubics' T values to segment
    Op::EdgeBuilder builder(one, contours);
    const int xorMask = builder.xorMask();
    builder.addOperand(two);
    builder.finish();
    const int xorOpMask = builder.xorMask();
    SkTDArray<Op::Contour*> contourList;
    makeContourList(contours, contourList, xorMask == kEvenOdd_Mask,
            xorOpMask == kEvenOdd_Mask);
    Op::Contour** currentPtr = contourList.begin();
    if (!currentPtr) {
        return;
    }
    Op::Contour** listEnd = contourList.end();
    // find all intersections between segments
    do {
        Op::Contour** nextPtr = currentPtr;
        Op::Contour* current = *currentPtr++;
        Op::Contour* next;
        do {
            next = *nextPtr++;
        } while (addIntersectTs(current, next) && nextPtr != listEnd);
    } while (currentPtr != listEnd);
    // eat through coincident edges

    int total = 0;
    int index;
    for (index = 0; index < contourList.count(); ++index) {
        total += contourList[index]->segments().count();
    }
#if DEBUG_SHOW_WINDING
    Op::Contour::debugShowWindingValues(contourList);
#endif
    coincidenceCheck(contourList, total);
#if DEBUG_SHOW_WINDING
    Op::Contour::debugShowWindingValues(contourList);
#endif
    fixOtherTIndex(contourList);
    sortSegments(contourList);
#if DEBUG_ACTIVE_SPANS
    debugShowActiveSpans(contourList);
#endif
    // construct closed contours
    Op::PathWrapper wrapper(result);
    bridgeOp(contourList, op, xorMask, xorOpMask, wrapper);
}
