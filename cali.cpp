/* -*- tab-width: 4 -*- */
/*****************************************************************************
 * Example code modified from Mapnik rundemo.cpp (r727, license LGPL). It
 * (1) Renders the State of California using USGS state boundaries data.
 * (2) Plots a set of California fourteeners as point symbols
 *****************************************************************************/

// define before any includes
#define BOOST_SPIRIT_THREADSAFE

#include <mapnik/map.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/font_engine_freetype.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/filter_factory.hpp>
#include <mapnik/color_factory.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/config_error.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/memory_datasource.hpp>
#include <mapnik/load_map.hpp>
#include <iostream>

// This class implements a simple way of displaying point-based data
//
class point_datasource : public mapnik::memory_datasource {
public:
	point_datasource() : feat_id(0) {}
	void add_point(double x, double y, const char* key, const char* value);
    int type() const { return mapnik::datasource::Raster; }

private:
	int feat_id;
};

void point_datasource::add_point(double x, double y, 
				 const char* key, const char* value)
{
	using namespace mapnik;
	feature_ptr feature(feature_factory::create(feat_id++));
	geometry2d * pt = new point_impl;
	pt->move_to(x,y);
	feature->add_geometry(pt);
	transcoder tr("utf-8");
	(*feature)[key] = tr.transcode(value);
	this->push(feature);
}

int main ( int argc , char** argv)
{
    if (argc != 2)
    {
        std::cout << "usage: $0 <mapnik_install_dir>\n";
        return EXIT_SUCCESS;
    }

    using namespace mapnik;
    try {
        std::string mapnik_dir(argv[1]);
        datasource_cache::instance()->register_datasources(mapnik_dir + "/plugins/input/shape");
        freetype_engine::register_font(mapnik_dir + "/fonts/dejavu-ttf-2.14/DejaVuSans.ttf");

        Map m(1080,680);
        m.set_background(Color(220, 226, 240));

		// Load Mountain-style from XML
		bool strict = true;
		mapnik::load_map(m, "style.xml", strict);

        // create styles

        // States (polygon)
        feature_type_style other_style;

        // non-CA states (polyline)
        feature_type_style provlines_style;

        stroke provlines_stk (Color(127,127,127),0.75);
        provlines_stk.add_dash(10, 6);

        rule_type provlines_rule;
        provlines_rule.append(polygon_symbolizer(color_factory::from_string("cornsilk")));
        provlines_rule.append(line_symbolizer(provlines_stk));
        provlines_rule.set_filter(create_filter("[STATE] <> 'California'"));
        other_style.add_rule(provlines_rule);

        m.insert_style("elsewhere", other_style);

        // Layers
        // Provincial  polygons
        {
            parameters p;
            p["type"]="shape";
            p["file"]="../data/statesp020"; // State Boundaries of the United States [SHP]

            Layer lyr("Cali");
            lyr.set_datasource(datasource_cache::instance()->create(p));
            lyr.add_style("cali"); // style.xml
            lyr.add_style("elsewhere"); // this file
            m.addLayer(lyr);
        }

		// Mountain data points
        {
			point_datasource* pds = new point_datasource;
			pds->add_point(-118.29, 36.58, "name", "mount Whitney");
			pds->add_point(-118.31, 36.65, "name", "mount Williamson");
			pds->add_point(-118.25, 37.63, "name", "White mountain");
			pds->add_point(-122.19, 41.41, "name", "mount Shasta");
			pds->add_point(-118.24, 37.52, "name", "mount Langley");
			// ...
			datasource_ptr ppoints_data(pds);

			// Plot datapoints
			Layer lyr("Mountains");
			lyr.set_datasource(ppoints_data);
			lyr.add_style("mtn");
			m.addLayer(lyr);
        }

        // Get layer's featureset
        Layer lay = m.getLayer(0);
        mapnik::datasource_ptr ds = lay.datasource();
        mapnik::query q(lay.envelope(), 1.0); // what does this second param 'res' do?
        q.add_property_name("STATE"); // NOTE: Without this call, no properties will be found!
        mapnik::featureset_ptr fs = ds->features(q);
        // One day, use filter_featureset<> instead?

        Envelope<double> extent; // NOTE: nil extent = <[0,0], [-1,-1]>

        // Loop through features in the set, get extent of those that match query && filter
        feature_ptr feat = fs->next();
        filter_ptr cali_f(create_filter("[STATE] = 'California'"));
        std::cerr << cali_f->to_string() << std::endl; // NOTE: prints (STATE=TODO)!
        while (feat) {
            if (cali_f->pass(*feat)) {
                for  (unsigned i=0; i<feat->num_geometries();++i) {
                    geometry2d & geom = feat->get_geometry(i);
                    if (extent.width() < 0 && extent.height() < 0) {
						// NOTE: expand_to_include() broken w/ nil extent. Work around.
                        extent = geom.envelope();
                    }
                    extent.expand_to_include(geom.envelope());
                }
            }
            feat = fs->next();
		}

        m.zoomToBox(extent);
        m.zoom(1.15); // zoom out slightly

        Image32 buf(m.getWidth(),m.getHeight());
        agg_renderer<Image32> ren(m,buf);
        ren.apply();

        save_to_file<ImageData32>(buf.data(),"cali.png","png");
    }
    catch ( const mapnik::config_error & ex )
    {
            std::cerr << "### Configuration error: " << ex.what();
            return EXIT_FAILURE;
    }
    catch ( const std::exception & ex )
    {
            std::cerr << "### std::exception: " << ex.what();
            return EXIT_FAILURE;
    }
    catch ( ... )
    {
            std::cerr << "### Unknown exception." << std::endl;
            return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
