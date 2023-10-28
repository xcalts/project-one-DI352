#include <cstdlib>
#include <string>
#include <iostream>

#define RYML_SINGLE_HDR_DEFINE_NOW

#include "argh.h"
#include "cluster.h"
#include "mnist.h"
#include "rapidyaml.h"

using namespace std;

#pragma region HELP_MESSAGE
const char *help_msg = R"""(
cluster - MNIST Image Clustering Tool

Usage: cluster [options] -i <input_file> -o <output_file> -c <configuration_file>

Description:
    The cluster tool is used to perform clustering on a dataset of MNIST images.
    It provides options for configuring the clustering process, including the
    assignment method and a configuration file.

Options:
    -h, --help
        Display this help message and exit.

    -m, --method lloyd
        Choose the assignment method for k-Means clustering. Options:
        - lloyd: Use Lloyd's assignment algorithm (default).
        - reverse: Use Reverse Search assignment algorithm.
        - lsh: Use LSH (Locality-Sensitive Hashing) assignment algorithm.
        - hypercube: Use the Hypercube assignment algorithm.

    -c, --configuration <configuration_file>
        Path to a configuration file.

Positional Arguments:
    -i, --input <input_file>
        Path to the MNIST dataset file.

    -o, --output <output_file>
        Path to the output file where clustering results will be saved.

Example Usage:
    cluster -m lloyd -i <input_file> -o <output_file> -c cluster.conf
    cluster -m reverse -i <input_file> -o <output_file> -c cluster.conf
    cluster -m lsh -i <input_file> -o <output_file> -c cluster.conf
    cluster -m hypercube -i <input_file> -o <output_file> -c cluster.conf

Note:
    - The MNIST dataset file should contain the MNIST images.
    - The tool will perform k-Means clustering on the MNIST dataset based on the
      provided configuration and save the clustered images in the specified output file.

Configuration File Format:
number_of_clusters: 4
number_of_vector_hash_tables: 3
number_of_vector_hash_functions: 4
max_number_M_hybercube: 10
number_of_hypercube_dimensions: 3
number_of_probes: 2

For more information, please refer to the documentation.
)""";
#pragma endregion

string readFileToString(const std::string &filename)
{
    ifstream file(filename);
    if (file)
    {
        // Read the entire file into a string
        string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return content;
    }
    else
    {
        // Handle the case where the file couldn't be opened
        throw runtime_error("Error opening the file: " + filename);
    }
}

int main(int argc, char *argv[])
{
    string input_file;
    string output_file;
    string conf_file;
    string method;
    string completion;
    int no_clusters;
    int no_hash_tables;
    int no_hash_functions;
    int no_max_hypercubes;
    int no_dim_hypercubes;
    int no_probes;

    argh::parser cmdl(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);
    cmdl({"-i", "--input"}) >> input_file;
    cmdl({"-o", "--output"}) >> output_file;
    cmdl({"-c", "--configuration"}) >> conf_file;
    cmdl({"-m", "--method"}) >> method;

    if (cmdl({"-h", "--help"}) || input_file.empty() || output_file.empty())
    {
        cout << help_msg << endl;
        return EXIT_FAILURE;
    }

    string conf_contents = readFileToString(conf_file);
    ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(conf_contents));

    tree["number_of_clusters"] >> no_clusters;
    tree["number_of_vector_hash_tables"] >> no_hash_tables;
    tree["number_of_vector_hash_functions"] >> no_hash_functions;
    tree["max_number_M_hybercube"] >> no_max_hypercubes;
    tree["number_of_hypercube_dimensions"] >> no_dim_hypercubes;
    tree["number_of_probes"] >> no_probes;

    MNIST input = MNIST(input_file);
    Cluster cluster = Cluster(no_clusters, no_hash_tables, no_hash_functions, no_max_hypercubes, no_dim_hypercubes, no_probes, input.GetImages());
    ofstream output(output_file, ios::out | ios::trunc);

    if (output.is_open())
    {
        output << cluster.getResults().rdbuf();

        output.close();
    }
    else
    {
        cout << "Failed to write to output file." << endl;
    }


}

vector<Image> InitializeCentroids(vector<Image> images, uint number_of_clusters)
{
    vector<Image> centroids(0);

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> random_index(0, images.size()-1);
    int indx = random_index(gen);

    // Get the first centroid randomly
    Image random_image = images[indx];
    centroids.push_back(random_image);

    for(int i = 0; i < number_of_clusters - 1; i++)
    { 
        // For every new centroid we want to initialize, calculate the points' min_dist_squared to the existing centroids
        vector<double> min_dists_squared(0);
        double min_dists_squared_sum = 0.0;
        
        for(int j = 0; j < images.size(); j++)
        {
            
            double min_dist = pow(2, 32) - 5;
            for(int k = 0; k < centroids.size(); k++)
            {
                double dist = CalculateDistance(2, images[j].GetImageData(), centroids[k].GetImageData());

                if(dist < min_dist) {
                    min_dist = dist;
                }
            }

            min_dists_squared.push_back(pow(min_dist, 2));
            min_dists_squared_sum += pow(min_dist, 2);
        }

        // Calculated probabilities based on squared min_dists, and min_dists_squared_sum
        vector<double> probabilities(0);
        double sum = 0;
        for(int j = 0; j < min_dists_squared.size(); j++)
        {
            sum += min_dists_squared[j] / min_dists_squared_sum;
            probabilities.push_back(min_dists_squared[j] / min_dists_squared_sum);
        }

        // Generate random real number between 0 and 1
        random_device rd;
        mt19937 gen(rd());
        uniform_real_distribution<> random_real(0.0, 1.0);

        double random_indx = random_real(gen);
        double cumulative_probability = 0.0;

        // Find out to which image this random_indx corresponds to
        for(int j = 0; j < probabilities.size(); j++)
        {
            cumulative_probability += probabilities[j];
            if(cumulative_probability >= random_indx) {
                centroids.push_back(images[j]);
                break;
            }
        }
    }

    return centroids;
}