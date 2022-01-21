//
//  options.hpp
//  macho-inject
//
//  Created by Jon Gabilondo on 20/03/2017.
//

#ifndef options_hpp
#define options_hpp

#include <stdio.h>

int add_options_description(boost::program_options::options_description & description);
int parse_options(int argc,
                  const char * argv[],
                  boost::program_options::options_description & description,
                  boost::program_options::variables_map &vm);

#endif /* options_hpp */
