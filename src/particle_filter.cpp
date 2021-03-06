/*
 * particle_filter.cpp
 *
 *  Created on: Dec 12, 2016
 *      Author: Tiffany Huang
 */

#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h> 
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include "particle_filter.h"

using namespace std;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
	// TODO: Set the number of particles. Initialize all particles to first position (based on estimates of 
	//   x, y, theta and their uncertainties from GPS) and all weights to 1. 
	// Add random Gaussian noise to each particle.
	// NOTE: Consult particle_filter.h for more information about this method (and others in this file).
	num_particles = 50;
	default_random_engine gen;
	normal_distribution<double> dist_x(x, std[0]);
	normal_distribution<double> dist_y(y, std[1]);
	normal_distribution<double> dist_theta(theta, std[2]);
	
	for(int i=0;i<num_particles;i++) {
		Particle p;
		p.id = i;
		p.x = dist_x(gen);
		p.y = dist_y(gen);
		p.theta = dist_theta(gen);
		p.weight = 1;
		particles.push_back(p);
		weights.push_back(p.weight);
	}
	is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
	// TODO: Add measurements to each particle and add random Gaussian noise.
	// NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
	//  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
	//  http://www.cplusplus.com/reference/random/default_random_engine/
	default_random_engine gen;
	for(int i=0;i<num_particles;i++) {
		
		if(yaw_rate == 0) {
			particles[i].x =  particles[i].x + velocity * delta_t * cos(particles[i].theta);
			particles[i].y =  particles[i].y + velocity * delta_t * sin(particles[i].theta);
			particles[i].theta =  particles[i].theta;
		}
		else {
			particles[i].x =  particles[i].x + (velocity/yaw_rate)*( sin(particles[i].theta+(yaw_rate * delta_t)) - sin(particles[i].theta) );
			particles[i].y =  particles[i].y + (velocity/yaw_rate)*( cos(particles[i].theta) - cos(particles[i].theta+(yaw_rate * delta_t)) );
			particles[i].theta =  particles[i].theta + (yaw_rate * delta_t); 
		}
	
		normal_distribution<double> dist_x(particles[i].x,std_pos[0]);
		normal_distribution<double> dist_y(particles[i].y,std_pos[1]);
		normal_distribution<double> dist_theta(particles[i].theta,std_pos[2]);
		particles[i].x = dist_x(gen);
		particles[i].y = dist_y(gen);
		particles[i].theta = dist_theta(gen);
	}
}

void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, std::vector<LandmarkObs>& observations) {
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the 
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to 
	//   implement this method and use it as a helper during the updateWeights phase.
	
}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
		const std::vector<LandmarkObs> &observations, const Map &map_landmarks) {
	// TODO: Update the weights of each particle using a multi-variate Gaussian distribution. You can read
	//   more about this distribution here: https://en.wikipedia.org/wiki/Multivariate_normal_distribution
	// NOTE: The observations are given in the VEHICLE'S coordinate system. Your particles are located
	//   according to the MAP'S coordinate system. You will need to transform between the two systems.
	//   Keep in mind that this transformation requires both rotation AND translation (but no scaling).
	//   The following is a good resource for the theory:
	//   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
	//   and the following is a good resource for the actual equation to implement (look at equation 
	//   3.33
	//   http://planning.cs.uiuc.edu/node99.html
	
	for(int j=0; j<num_particles;j++) {
		vector<LandmarkObs> obs_landmarks;
		vector<int> associations;
		vector<double> sense_x;
		vector<double> sense_y;
		
		//Transform observations from particle coordinates to global coordinates
		for(int i=0;i<observations.size();i++) {
			LandmarkObs l;
			l.x = particles[j].x + (observations[i].x * cos(particles[j].theta)) - (sin(particles[j].theta) * observations[i].y);
			l.y = particles[j].y + (observations[i].x * sin(particles[j].theta)) + (cos(particles[j].theta) * observations[i].y);
			obs_landmarks.push_back(l);
		}
		particles[j].weight = 1;
		
		for(int i =0;i<obs_landmarks.size();i++) {
			double x = obs_landmarks[i].x;
			double y = obs_landmarks[i].y;
			double shortest_dist = sqrt( pow((x - map_landmarks.landmark_list[0].x_f),2) + pow((y - map_landmarks.landmark_list[0].y_f),2) );
			int id = 0;
			for(int k=1;k<map_landmarks.landmark_list.size();k++) {
				double dist = sqrt( pow((x - map_landmarks.landmark_list[k].x_f),2) + pow((y - map_landmarks.landmark_list[k].y_f),2) );
				if(dist<shortest_dist) {
					shortest_dist = dist; 
					id = k; 
				}
			}
			double mu_x = map_landmarks.landmark_list[id].x_f;
			double mu_y = map_landmarks.landmark_list[id].y_f;
			double multi_dist = (1/(2*M_PI*std_landmark[0]*std_landmark[1])) * exp(-1*( (pow((x - mu_x),2)/(2*pow(std_landmark[0],2))) + (pow((y - mu_y),2)/(2*pow(std_landmark[1],2))) ));
			particles[j].weight*=multi_dist;
			associations.push_back(id+1); 
			sense_x.push_back(x);
			sense_y.push_back(y);
		}
		weights[j]=particles[j].weight;
		SetAssociations(particles[j], associations, sense_x, sense_y);
	}
}

void ParticleFilter::resample() {
	// TODO: Resample particles with replacement with probability proportional to their weight. 
	// NOTE: You may find std::discrete_distribution helpful here.
	//   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
	
	default_random_engine gen;
	discrete_distribution<int> sample_select(weights.begin(),weights.end());
	std::vector<Particle> resample_particles;
	
	for(int i=0; i < num_particles ; i++) {
			resample_particles.push_back(particles[sample_select(gen)]);
	}
	
	particles = resample_particles;
}

void ParticleFilter::SetAssociations(Particle& particle, const std::vector<int>& associations, 
                                     const std::vector<double>& sense_x, const std::vector<double>& sense_y)
{
    //particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
    // associations: The landmark id that goes along with each listed association
    // sense_x: the associations x mapping already converted to world coordinates
    // sense_y: the associations y mapping already converted to world coordinates

    particle.associations= associations;
    particle.sense_x = sense_x;
    particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best)
{
	vector<int> v = best.associations;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<int>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseX(Particle best)
{
	vector<double> v = best.sense_x;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseY(Particle best)
{
	vector<double> v = best.sense_y;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
