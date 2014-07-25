/*
 * Draw.cc
 *
 * Handling path drawing
 *
 * by WangLu
 * 2012.10.01
 */

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>
#include <iostream>

#include "HTMLRenderer.h"
#include "util/misc.h"
#include "util/math.h"
#include "util/namespace.h"

namespace pdf2htmlEX {

using std::swap;
using std::min;
using std::max;
using std::acos;
using std::asin;
using std::ostringstream;
using std::sqrt;
using std::vector;
using std::ostream;

void HTMLRenderer::restoreState(GfxState * state)
{
    updateAll(state);
    tracer.restore();
}

void HTMLRenderer::saveState(GfxState *state)
{
    tracer.save();
}

void HTMLRenderer::stroke(GfxState * state)
{
    tracer.stroke(state);
}

void HTMLRenderer::fill(GfxState * state)
{
    tracer.fill(state);
}

void HTMLRenderer::eoFill(GfxState * state)
{
    tracer.fill(state, true);
}

GBool HTMLRenderer::axialShadedFill(GfxState *state, GfxAxialShading *shading, double tMin, double tMax)
{
    tracer.fill(state); //TODO correct?
    return true;
}

} // namespace pdf2htmlEX
