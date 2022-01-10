#include <ut/ut.hpp>
#include <utility>
#include "config/config.h"
#include "log/logging.h"
#include "config/common.h"
#include "config/merging.h"

namespace ut = boost::ut;

ut::suite base_config = [] {
    using namespace ut;
    using namespace tasarch::config;

    "shared instance testing"_test = []{
        auto conf = tasarch::config::conf();
        expect(conf->testing == false);

        auto capture_by_val = [=]{
            expect(conf->testing == true);
        };

        auto capture_by_ref = [&]{
            expect(conf->testing == true);
            conf->testing = false;
            expect(conf->testing == false);
        };

        auto argument_val = [](conf_ptr conf) {
            expect(conf->testing == true);
            conf->testing = false;
            expect(conf->testing == false);
        };

        auto argument_ref = [](conf_ptr& conf) {
            expect(conf->testing == true);
            conf->testing = false;
            expect(conf->testing == false);
        };

        conf->testing = true;
        expect(conf->testing == true);
        expect(config::instance()->testing == true);

        capture_by_val();
        capture_by_ref();
        
        conf->testing = true;

        expect(config::instance()->testing == true);
        argument_ref(conf);
        expect(config::instance()->testing == false);

        conf->testing = true;

        expect(config::instance()->testing == true);
        argument_val(conf);
        expect(config::instance()->testing == false);
    };
};