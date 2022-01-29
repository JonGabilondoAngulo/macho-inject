//
//  options.hpp
//  macho-inject
//
//  Created by Jon Gabilondo on 20/03/2017.
//

#ifndef cmd_options_hpp
#define cmd_options_hpp

#include <stdio.h>

int cmd_opt_add_descriptions(boost::program_options::options_description & description);
int cmd_opt_parse_options(int argc,
                          const char * argv[],
                          boost::program_options::options_description & description,
                          boost::program_options::variables_map &vm);

#endif /* options_hpp */
