//#include <vector>
//#include <map>
//#include <fstream>
//#include <sstream>
//#include <filesystem>
//#include <cassert>
//#include <numeric>
//#include <iterator>
//#include "gtest/gtest.h"

//#include "prefix.hpp"
//#include "words_count.hpp"

//using pair_t = mapreduce::Framework::pair_t;
//using pairs_t = mapreduce::Framework::pairs_t;
//using blocks_of_pairs_t = mapreduce::Framework::blocks_of_pairs_t;

//TEST(TEST_PREFIX, mr_prefix)
//{
//    std::filesystem::path input("../tests/prefix-input.txt");
//    std::filesystem::path output("../tests/prefix-out.txt");

//    auto check = [&output]()
//    {
//        std::fstream file{output.string(), std::ios::in};
//        std::string prefix;
//        //std::getline(file, prefix, ' ');
//        file >> prefix;
//        std::cout << "prefix: " << prefix << std::endl;
//        std::string length;
//        //std::getline(file, length, '\n');
//        file >> length;
//        ASSERT_TRUE(prefix == "anabanbanana" || prefix == "bananabanana");
//        EXPECT_EQ(length, "12");
//    };

//    {
//        std::cout << 1 << ' ' << 1 << std::endl;
//        mapreduce::Framework mr{mapreduce_prefix::mapper, 1, mapreduce_prefix::reducer, 1};
//        mr.run(input, output);
//        check();
//    }
//    {
//        std::cout << 3 << ' ' << 2 << std::endl;
//        mapreduce::Framework mr{mapreduce_prefix::mapper, 3, mapreduce_prefix::reducer, 2};
//        mr.run(input, output);
//        check();
//    }
//    {
//        std::cout << 5 << ' ' << 3 << std::endl;
//        mapreduce::Framework mr{mapreduce_prefix::mapper, 5, mapreduce_prefix::reducer, 3};
//        mr.run(input, output);
//        check();
//    }
//    {
//        std::cout << 10 << ' ' << 3 << std::endl;
//        mapreduce::Framework mr{mapreduce_prefix::mapper, 10, mapreduce_prefix::reducer, 3};
//        mr.run(input, output);
//        check();
//    }
//    {
//        std::cout << 1 << ' ' << 2 << std::endl;
//        mapreduce::Framework mr{mapreduce_prefix::mapper, 1, mapreduce_prefix::reducer, 2};
//        mr.run(input, output);
//        check();
//    }
//    {
//        std::cout << 5 << ' ' << 8 << std::endl;
//        mapreduce::Framework mr{mapreduce_prefix::mapper, 5, mapreduce_prefix::reducer, 8};
//        mr.run(input, output);
//        check();
//    }
//    {
//        std::cout << 20 << ' ' << 8 << std::endl;
//        mapreduce::Framework mr{mapreduce_prefix::mapper, 20, mapreduce_prefix::reducer, 8};
//        mr.run(input, output);
//        check();
//    }
//}

//TEST(TEST_PREFIX, mr_word_count)
//{
//    std::filesystem::path input("../tests/wcount-input.txt");
//    std::filesystem::path output("../tests/wcount-out.txt");

//    auto check = [&output]()
//    {
//        std::filesystem::path input_ref("../tests/wcount-classical-out.txt");
//        std::fstream file_ref{input_ref.string(), std::ios::in};
//        std::fstream file{output.string(), std::ios::in};
////        EXPECT_EQ(std::filesystem::file_size(output), std::filesystem::file_size(input_ref));

//        std::string ref;
//        for(std::string s; std::getline(file_ref, s);)
//            ref += std::move(s);

//        std::string res;
//        for(std::string s; std::getline(file, s);)
//            res += std::move(s);

//        std::cout << ref << std::endl;
//        std::cout << res << std::endl;

//        EXPECT_EQ(res, ref);
//    };

//    mapreduce_words_count::classical();

//    {
//        std::cout << 1 << ' ' << 1 << std::endl;
//        mapreduce::Framework mr{mapreduce_words_count::mapper, 1, mapreduce_words_count::reducer, 1};
//        mr.run(input, output);
//        check();
//    }
//    {
//        std::cout << 4 << ' ' << 3 << std::endl;
//        mapreduce::Framework mr{mapreduce_words_count::mapper, 4, mapreduce_words_count::reducer, 3};
//        mr.run(input, output);
//        check();
//    }
//    {
//        std::cout << 2 << ' ' << 1 << std::endl;
//        mapreduce::Framework mr{mapreduce_words_count::mapper, 2, mapreduce_words_count::reducer, 1};
//        mr.run(input, output);
//        check();
//    }
//    {
//        std::cout << 10 << ' ' << 5 << std::endl;
//        mapreduce::Framework mr{mapreduce_words_count::mapper, 10, mapreduce_words_count::reducer, 5};
//        mr.run(input, output);
//        check();
//    }
//    {
//        std::cout << 20 << ' ' << 7 << std::endl;
//        mapreduce::Framework mr{mapreduce_words_count::mapper, 20, mapreduce_words_count::reducer, 7};
//        mr.run(input, output);
//        check();
//    }
//    {
//        std::cout << 5 << ' ' << 10 << std::endl;
//        mapreduce::Framework mr{mapreduce_words_count::mapper, 5, mapreduce_words_count::reducer, 10};
//        mr.run(input, output);
//        check();
//    }
//    {
//        std::cout << 2 << ' ' << 5 << std::endl;
//        mapreduce::Framework mr{mapreduce_words_count::mapper, 2, mapreduce_words_count::reducer, 5};
//        mr.run(input, output);
//        check();
//    }
//}


//int main(int argc, char** argv)
//{
//    ::testing::InitGoogleTest(&argc, argv);
//    return RUN_ALL_TESTS();
//}
