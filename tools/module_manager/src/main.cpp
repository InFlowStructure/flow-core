#include <Zipper/Unzipper.hpp>
#include <cxxopts.hpp>
#include <flow/core/Module.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

using json = nlohmann::json;
using namespace zipper;

int main(int argc, char** argv)
{
    // clang-format off
    cxxopts::Options options("FlowModuleManager");
    options.add_options()
        ("f,file", "Flow file to open", cxxopts::value<std::filesystem::path>())
        ("h,help", "Print usage");
    // clang-format on

    cxxopts::ParseResult result;

    try
    {
        result = options.parse(argc, argv);
    }
    catch (const cxxopts::exceptions::exception& e)
    {
        std::cerr << "Caught exception while parsing arguments: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    if (result.count("help"))
    {
        std::cerr << options.help() << std::endl;
        return EXIT_SUCCESS;
    }

    if (result.count("file") == 0)
    {
        std::cerr << "No fmod file provided" << std::endl;
        return EXIT_FAILURE;
    }

    const auto temp_path = std::filesystem::temp_directory_path() / "tmp_flow_modules";

    try
    {
        std::filesystem::path fmod_file_path = result["file"].as<std::filesystem::path>();
        if (!std::filesystem::exists(fmod_file_path) || !std::filesystem::is_regular_file(fmod_file_path))
        {
            std::cerr << fmod_file_path.string() << " is not a file" << std::endl;
            return EXIT_FAILURE;
        }

        Unzipper unzipper(fmod_file_path.string());
        if (!unzipper.isOpened())
        {
            throw std::runtime_error(std::format("Failed to open module archive. (file={})", fmod_file_path.string()));
        }

        unzipper.extractAll((temp_path).string(), Unzipper::OverwriteMode::Overwrite);
        unzipper.close();

        flow::ModuleMetaData::Validate(json::parse(std::ifstream(temp_path / fmod_file_path.stem() / "module.json")));
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}