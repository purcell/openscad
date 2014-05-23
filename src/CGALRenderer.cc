/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifdef _MSC_VER 
// Boost conflicts with MPFR under MSVC (google it)
#include <mpfr.h>
#endif

// dxfdata.h must come first for Eigen SIMD alignment issues
#include "dxfdata.h"
#include "polyset.h"
#include "printutils.h"

#include "CGALRenderer.h"
#include "CGAL_renderer.h"
#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"

//#include "Preferences.h"

CGALRenderer::CGALRenderer(shared_ptr<const class Geometry> geom)
{
	if (shared_ptr<const PolySet> ps = dynamic_pointer_cast<const PolySet>(geom)) {
		this->polyset = ps;
	}
	else if (shared_ptr<const Polygon2d> poly = dynamic_pointer_cast<const Polygon2d>(geom)) {
		this->polyset.reset(poly->tessellate());
	}
	else if (shared_ptr<const CGAL_Nef_polyhedron> new_N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
		assert(new_N->getDimension() == 3);
		if (!new_N->isEmpty()) {
			this->N = new_N;
			this->rebuildPolyhedron();
		}
	}
}

CGALRenderer::~CGALRenderer()
{
}

void CGALRenderer::rebuildPolyhedron()
{
	PRINT("cgr rebuild poly");
	if (this->N) {
		this->polyhedron.reset(new Polyhedron());
		if (this->colorscheme) this->polyhedron->setColorScheme(*this->colorscheme);
		CGAL::OGL::Nef3_Converter<CGAL_Nef_polyhedron3>::convert_to_OGLPolyhedron(*this->N->p3, this->polyhedron.get());
		// CGAL_NEF3_MARKED_FACET_COLOR <- CGAL_FACE_BACK_COLOR
		// CGAL_NEF3_UNMARKED_FACET_COLOR <- CGAL_FACE_FRONT_COLOR
		this->polyhedron->init();
	}
	PRINT("cgr rebuild end");
}

void CGALRenderer::setColorScheme(const OSColors::colorscheme &cs)
{
	PRINT("Cgalrenderer scholor");
	Renderer::setColorScheme(cs);
	this->rebuildPolyhedron();
	PRINT("Cgalrenderer scholor done");
}

void CGALRenderer::draw(bool showfaces, bool showedges) const
{
	PRINT("cgalrenderer draw");
	if (this->polyset) {
		if (this->polyset->getDimension() == 2) {
			PRINT("cgalrenderer draw 2D");
			// Draw 2D polygons
			glDisable(GL_LIGHTING);
// FIXME:		const QColor &col = Preferences::inst()->color(Preferences::CGAL_FACE_2D_COLOR);
			glColor3f(0.0f, 0.75f, 0.60f);

			for (size_t i=0; i < this->polyset->polygons.size(); i++) {
				glBegin(GL_POLYGON);
				for (size_t j=0; j < this->polyset->polygons[i].size(); j++) {
					const Vector3d &p = this->polyset->polygons[i][j];
					glVertex3d(p[0], p[1], -0.1);
				}
				glEnd();
			}
		
			// Draw 2D edges
			glDisable(GL_DEPTH_TEST);

			glLineWidth(2);
// FIXME:		const QColor &col2 = Preferences::inst()->color(Preferences::CGAL_EDGE_2D_COLOR);
			glColor3f(1.0f, 0.0f, 0.0f);
			this->polyset->render_edges(CSGMODE_NONE);
			glEnable(GL_DEPTH_TEST);
		}
		else {
			PRINT("cgalrenderer polyset draw");
			// Draw 3D polygons
			const Color4f c(-1,-1,-1,-1);	
			setColor(COLORMODE_MATERIAL, c.data(), NULL);
			this->polyset->render_surface(CSGMODE_NORMAL, Transform3d::Identity(), NULL);
		}
	}
	else if (this->polyhedron) {
		PRINT("cgalrenderer polyhedron draw");
		if (showfaces) this->polyhedron->set_style(SNC_BOUNDARY);
		else this->polyhedron->set_style(SNC_SKELETON);
		
		this->polyhedron->draw(showfaces && showedges);
  }
}
