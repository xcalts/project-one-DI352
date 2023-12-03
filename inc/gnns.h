#ifndef GNNS_H
#define GNNS_H

#include <vector>
#include <list>

#include "lsh.h"

#define GREEDY_STEPS 20

using namespace std;

// GNNS contains the functionality of the Graph Nearest Neighbor Search algorithm.
class GNNS
{
private:
    int no_lsh_neighbors;                                         // Number of LSH nearest neighbors to use (default: 40).
    int no_expansions;                                            // Number of expansions to use (default: 30).
    int no_restarts;                                              // Number of random restarts (default: 1).
    MNIST input;                                                  // The MNIST dataset's images converted to d-vectors.
    list<int> *graph;                                             // Graph implementation using adjacency list.

public:
    // Create a new instance of GNNS.
    GNNS(MNIST _input, int _no_lsh_neighbors, int _no_expansions, int _no_restarts)
    {
        no_lsh_neighbors = _no_lsh_neighbors;
        no_expansions = _no_expansions;
        no_restarts = _no_restarts;
        input = _input;

        graph = new list<int>[GNNS::input.GetImagesCount()];
    }

    void Initialization()
    {
        cout << "DEBUG: Create graph using LSH." << endl;

        LSH lsh = LSH(input, 10, 15);

        vector<MNIST_Image> images = input.GetImages();

        // cout << no_lsh_neighbors << endl;
        // for each image in input find a set of nearest neighbors
        for (MNIST_Image query_image : images)
        {
            set<MNIST_Image, MNIST_ImageComparator> lsh_nn = lsh.FindNearestNeighbors(no_lsh_neighbors, query_image);

            // loop through the set of nearest neighbors
            for (auto it1 = lsh_nn.begin();
                 it1 != lsh_nn.end();
                 it1++)
            {
                MNIST_Image neighbor_lsh = *it1;
                
                if (query_image.GetIndex() == 1) {
                    cout << neighbor_lsh.GetIndex() << "\n";
                }

                graph[query_image.GetIndex()].push_back(neighbor_lsh.GetIndex());
            }
            
        }

        cout << "DEBUG: Finished creating graph using LSH." << endl;
    }

    set<MNIST_Image, MNIST_ImageComparator> FindNearestNeighbors(int no_nearest_neighbours, MNIST_Image query_image)
    {   
        vector<MNIST_Image> images = input.GetImages();

        set<MNIST_Image, MNIST_ImageComparator> possible_nearest_neighbors; // Used for sorting the final vector of ANN

        random_device rd;
        mt19937 gen(rd());

        uniform_int_distribution<int> random_image_index(0, input.GetImagesCount());

        for (int i = 0; i < no_restarts; i++)
        {   

            cout << "Restart: " << i << "\n";
            // Select a graph's node to start at random
            int index = random_image_index(gen);         
            MNIST_Image node_image = images[index];

            double min_dist = EuclideanDistance(2, query_image.GetImageData(), node_image.GetImageData());
            node_image.SetDist(min_dist);  
            
            // Insert starting node to nearest_neighbors
            possible_nearest_neighbors.insert(node_image);

            // Execute t greedy steps
            for (int t = 0; t < GREEDY_STEPS; t++)
            {
                cout << "Step: " << t << "\n";
            
                int exp = 0;                 // Number of expansions so far
                int curr_nn = -1;            // Symbolizes the index of the expanded node with min distance to the query

                // Execute no_expansions expansions
                for (int neighbor_index : graph[index])
                {
                    MNIST_Image neighbor_image = images[neighbor_index];
                    double dist = EuclideanDistance(2, query_image.GetImageData(), neighbor_image.GetImageData());
                    neighbor_image.SetDist(dist);

                    possible_nearest_neighbors.insert(neighbor_image);

                    // Mark the next graph node to be expanded
                    if (dist < min_dist)
                    {
                        min_dist = dist;
                        curr_nn = neighbor_index;
                    }

                    exp++;
                    if (no_expansions == exp)
                    {
                        break;
                    }
                }

                // In case we reached a local minimum.
                if (curr_nn == -1)
                {
                    cout << "Reached local min. \n";
                    break;
                } 

                // Else, we use the current nearest neighbor to the query as the next node to expand
                index = curr_nn;
                node_image = images[curr_nn];
                double dist = EuclideanDistance(2, query_image.GetImageData(), node_image.GetImageData());
                node_image.SetDist(dist);

            }

        }

        set<MNIST_Image, MNIST_ImageComparator> nearest_neighbors;

        int nn_num = 0;
        for (MNIST_Image nn : possible_nearest_neighbors)
        {
            if(nn_num == no_nearest_neighbours)
            {
                break;
            }

            nearest_neighbors.insert(nn);
            nn_num++;
        }

        return nearest_neighbors;
    }

    void PrintGraph()
    {
        cout << "DEBUG: Printing Graph." << endl;

        vector<MNIST_Image> images = input.GetImages();

        for (MNIST_Image query_image : images)
        {
            cout << query_image.GetIndex() << "\n";
            
            for(int i : graph[query_image.GetIndex()])
            {
                cout << "Edge(" << query_image.GetIndex() << ", " << i << ")\n";
            }
        }

        cout << "DEBUG: Finished printing Graph." << endl;
    }

};

#endif // GNNS_H