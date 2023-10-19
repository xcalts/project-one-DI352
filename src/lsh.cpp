#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <unordered_map>
#include <cstring>
#include <ctime>

#include "mnist.h"
#include "hashing.h"
#include "image.h"
#include "lsh.h"

using namespace std;

#define WINDOW 400

/* Constructor */
LSH::LSH(MNIST input, MNIST query, string output, int k, int L, int N, int R)
{
    output_file_path = output;
    number_of_hashing_functions = k;
    number_of_hash_tables = L;
    number_of_nearest = N;
    radius = R;
    query_images = query.GetImages();
    images = input.GetImages();

    hash_tables = vector<unordered_map<uint, vector<Image>>>(L);
}

/* Functions */
void LSH::HashInput()
{
    // for each hash table, use it's hashing functions and hash the input dataset
    cout << "Hashing the input set into buckets..." << endl;

    // create random projection vectors
    random_projections = GetRandomProjections(number_of_hash_tables, number_of_hashing_functions);

    for(int i = 0; i < number_of_hash_tables; i++) {   // For each hash table

        for(int j = 0; j < (int) images.size(); j++) {       // For each image in input set
            uint hash_code_for_querying_trick = CalculateFinalHashCode(images[j].GetImageData(), random_projections[i], number_of_hashing_functions, WINDOW);
            uint final_hash_code = hash_code_for_querying_trick % ((uint) (images.size() / 16));

            hash_tables[i][final_hash_code].push_back(images[j]);   
        }
    }
}

set<Image, ImageComparator> LSH::FindAllNearestNeighbors(Image query_image)
{
    cout << "Searching for " << number_of_nearest << " Nearest Neighbors using LSH..." << endl;

    set<Image, ImageComparator> nearest_neighbors;
    double min_dist = pow(2, 32) - 5;
    
    for(int i = 0; i < number_of_hash_tables; i++) {    // For each hash table

        //Find query_image's hash code for given table
        uint hash_code_for_querying_trick = CalculateFinalHashCode(query_image.GetImageData(), random_projections[i], number_of_hashing_functions, WINDOW);
        uint final_hash_code = hash_code_for_querying_trick % ((uint) (images.size() / 16));

        if(hash_tables[i][final_hash_code].size() == 0) {
            continue;
        }

        vector<Image> bucket_images = hash_tables[i][final_hash_code];

        for(int j = 0; j < (int) bucket_images.size(); j++) {   // For each image found in the same bucket as query_image
            double dist = CalculateDistance(2, query_image.GetImageData(), bucket_images[j].GetImageData());
            bucket_images[j].SetDist(dist);
            
            if(dist < min_dist) {
                if((int) nearest_neighbors.size() == number_of_nearest) {
                    nearest_neighbors.erase(--nearest_neighbors.end());
                }

                nearest_neighbors.insert(bucket_images[j]);

                set<Image, ImageComparator>::reverse_iterator itr = nearest_neighbors.rbegin();
                Image last_image = *itr;
                min_dist = last_image.GetDist();
            }
            
        }
    }

    return nearest_neighbors;
}

set<Image, ImageComparator> LSH::BruteForceNearestNeighbors(Image query_image)
{
    cout << "Searching for " << number_of_nearest << " Nearest Neighbors using Brute Force..." << endl;

    set<Image, ImageComparator> nearest_neighbors;
    double min_dist = pow(2, 32) - 5;

    for(int i = 0; i < (int) images.size(); i++) {
        double dist = CalculateDistance(2, query_image.GetImageData(), images[i].GetImageData());
        images[i].SetDist(dist);
        
        if(dist < min_dist) {
            if((int) nearest_neighbors.size() == number_of_nearest) {
                nearest_neighbors.erase(--nearest_neighbors.end());
            }

            nearest_neighbors.insert(images[i]);

            set<Image, ImageComparator>::reverse_iterator itr = nearest_neighbors.rbegin();
            Image last_image = *itr;
            min_dist = last_image.GetDist();
        }
    }

    return nearest_neighbors;
}

set<Image, ImageComparator> LSH::RadiusSearch(Image query_image)
{
    cout << "Searching for Neighbors in Radius using LSH..." << endl;

    set<Image, ImageComparator> nearest_neighbors;
    
    for(int i = 0; i < number_of_hash_tables; i++) {    // For each hash table

        //Find query_image's hash code for given table
        uint hash_code_for_querying_trick = CalculateFinalHashCode(query_image.GetImageData(), random_projections[i], number_of_hashing_functions, WINDOW);
        uint final_hash_code = hash_code_for_querying_trick % ((uint) (images.size() / 16));

        if(hash_tables[i][final_hash_code].size() == 0) {
            continue;
        }

        vector<Image> bucket_images = hash_tables[i][final_hash_code];

        for(int j = 0; j < (int) bucket_images.size(); j++) {   // For each image found in the same bucket as query_image
            double dist = CalculateDistance(2, query_image.GetImageData(), bucket_images[j].GetImageData());
            bucket_images[j].SetDist(dist);
            
            if(dist < radius) {
                nearest_neighbors.insert(bucket_images[j]);
            }
            
        }
    }

    return nearest_neighbors;
}

void LSH::Execute()
{
    HashInput();

    for(int i = 0; i < (int) query_images.size(); i++) {     // For each image in the query set

        const clock_t lsh_begin = clock();
        set<Image, ImageComparator> nearest_neighbors_lsh = FindAllNearestNeighbors(query_images[i]);
        const clock_t lsh_end = clock();

        double lsh_time = double(lsh_end - lsh_begin) / CLOCKS_PER_SEC;

        cout << "Time for LSH: " << lsh_time << endl;

        /*
        Used for debugging

        for (set<Image>::iterator it = nearest_neighbors_lsh.begin(); it != nearest_neighbors_lsh.end(); ++it) {
            Image element = *it;

            for (uint32_t i = 0; i < 28; ++i)
            {
                for (uint32_t j = 0; j < 28; ++j)
                {
                    int pixelValue = element.GetImageData()[i * 28 + j];
                    char displayChar = '#';

                    // Use ' ' for white and '#' for black based on the pixel value
                    if (pixelValue < 128)
                    {
                        displayChar = ' '; // Black
                    }

                    std::cout << displayChar;
                }
                std::cout << std::endl;
            }
            std::cout << element.GetDist() << endl;
        } */
    
        const clock_t brute_begin = clock();
        set<Image, ImageComparator> nearest_neighbors_brute_force = BruteForceNearestNeighbors(query_images[i]);
        const clock_t brute_end = clock();

        double brute_time = double(brute_end - brute_begin) / CLOCKS_PER_SEC;

        cout << "Time for Brute Force: " << brute_time << endl;

        /*
        Used for debugging

        for (set<Image>::iterator it = nearest_neighbors_brute_force.begin(); it != nearest_neighbors_brute_force.end(); ++it) {
            Image element = *it;

            for (uint32_t i = 0; i < 28; ++i)
            {
                for (uint32_t j = 0; j < 28; ++j)
                {
                    int pixelValue = element.GetImageData()[i * 28 + j];
                    char displayChar = '#';

                    // Use ' ' for white and '#' for black based on the pixel value
                    if (pixelValue < 128)
                    {
                        displayChar = ' '; // Black
                    }

                    std::cout << displayChar;
                }
                std::cout << std::endl;
            }
            std::cout << element.GetDist() << endl;
        } */

        set<Image, ImageComparator> neighbors_in_radius = RadiusSearch(query_images[i]);

        cout << "Neihbours in Radius " << radius << ": " << neighbors_in_radius.size() << endl;

        /*
        Used for debugging

        for (uint32_t i = 0; i < 28; ++i)
        {
            for (uint32_t j = 0; j < 28; ++j)
            {
                int pixelValue = query_images[4].GetImageData()[i * 28 + j];
                char displayChar = '#';

                // Use ' ' for white and '#' for black based on the pixel value
                if (pixelValue < 128)
                {
                    displayChar = ' '; // Black
                }

                std::cout << displayChar;
            }
            std::cout << std::endl;
        } */

        WriteToFile(query_images[i], nearest_neighbors_lsh, nearest_neighbors_brute_force, neighbors_in_radius, lsh_time, brute_time);
    }
}


void LSH::Print()
{
    cout << "Output File Path:                        " << output_file_path << endl;
    cout << "Number of Hashing Functions:             " << to_string(number_of_hashing_functions) << endl;
    cout << "Number of Hash Tables:                   " << to_string(number_of_hash_tables) << endl;
    cout << "Number of Nearest Neighbors to be Found: " << to_string(number_of_nearest) << endl;
    cout << "Radius:                                  " << to_string(radius) << endl;
}

void LSH::WriteToFile(Image query_image, set<Image, ImageComparator> nearest_neighbors_lsh, set<Image, ImageComparator> nearest_neighbors_brute_force, set<Image, ImageComparator> neighbors_in_radius, double lsh_time, double brute_time) {
    ofstream output(output_file_path, ios::app);

    if(output.is_open()) {

        output << "Query: " << query_image.GetIndex() << endl;

        // write data for LSH and Brute Force
        int i = 1;
        for(set<Image, ImageComparator>::iterator it1 = nearest_neighbors_lsh.begin(), it2 = nearest_neighbors_brute_force.begin();
            (it1 != nearest_neighbors_lsh.end()) && (it2 != nearest_neighbors_brute_force.end());
            ++it1, ++it2) 
        {
            Image neighbor_lsh = *it1;
            Image neighbor_brute = *it2;

            output << "Nearest neighbor-" << i << ": " << neighbor_lsh.GetIndex() << endl;
            output << "distanceLSH: " << neighbor_lsh.GetDist() << endl;
            output << "distanceTrue: " << neighbor_brute.GetDist() << endl;

            i++;
        }

        // for(int i = 0; i < (int) nearest_neighbors_lsh.size(); i++) {
        //     output << "Nearest neighbor-" << i << ": " << nearest_neighbors_lsh[i].GetIndex() << endl;
        //     output << "distanceLSH: " << nearest_neighbors_lsh[i].GetDist() << endl;
        //     output << "distanceTrue: " << nearest_neighbors_brute_force[i].GetDist() << endl;
        // }

        output << "tLSH: " << lsh_time << endl;
        output << "tTrue: " << brute_time << endl;

        //write data for radius search
        output << "R-near neighbors:" << endl;
        for (set<Image, ImageComparator>::iterator it = neighbors_in_radius.begin(); it != neighbors_in_radius.end(); ++it) {
            Image neighbor = *it;
            output << neighbor.GetIndex() << endl;
        }

        output.close();
    } else {
        cout << "Failed to write to output file." << endl;
    }
}