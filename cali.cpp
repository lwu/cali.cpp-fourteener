/*****************************************************************************
 * Example code modified from Mapnik rundemo.cpp (r727, license LGPL).
 * Renders the State of California using USGS state boundaries data.
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
#include <iostream>

class point_datasource : public memory_datasource {
public:
	point_datasource() : feat_id(0) {}
	void add_point(double x,y double y, string value);
private:
	int feat_id;
};

void point_datasource::add_point(double x, double y, string value)
{
	feature_ptr feature(feature_factory::create(feat_id++));
	feature->set_geometry();
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

        // create styles

        // States (polygon)
        feature_type_style cali_style;

        rule_type cali_rule;
        filter_ptr cali_filter = create_filter("[STATE] = 'California'");
        cali_rule.set_filter(cali_filter);
        cali_rule.append(polygon_symbolizer(Color(207, 222, 167)));
        cali_style.add_rule(cali_rule);

        // Provinces (polyline)
        feature_type_style provlines_style;

        stroke provlines_stk (Color(127,127,127),0.75);
        provlines_stk.add_dash(10, 6);

        rule_type provlines_rule;
        provlines_rule.append(polygon_symbolizer(color_factory::from_string("cornsilk")));
        provlines_rule.append(line_symbolizer(provlines_stk));
        provlines_rule.set_filter(create_filter("[STATE] <> 'California'"));
        cali_style.add_rule(provlines_rule);

        m.insert_style("cali",cali_style);

        // Layers
        // Provincial  polygons
        {
            parameters p;
            p["type"]="shape";
            p["file"]="../data/statesp020"; // State Boundaries of the United States [SHP]

            Layer lyr("Cali");
            lyr.set_datasource(datasource_cache::instance()->create(p));
            lyr.add_style("cali");
            m.addLayer(lyr);
        }

        {
			parameters p;
			p["type"]="shape";
			p["file"]="..";
        }

	// Plot datapoints
	point_datasource pds;

        // Get layer's featureset
        Layer lay = m.getLayer(0);
        mapnik::datasource_ptr ds = lay.datasource();
        mapnik::query q(lay.envelope(), 1.0 /* what does this "res" param do? */);
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
        m.zoom(1.2); // zoom out slightly

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
