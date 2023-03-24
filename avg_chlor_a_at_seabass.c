#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <netcdf.h>

#define NUM_MEDIAN_SAMPLES 32

typedef struct vec2 {
	size_t lat, lon;
} vec2;

typedef struct node {
	float location;
	vec2 idx; // the idxs of the nc data, only exists on leaves
	void *left, *right;
} node;


int fcompare(const void *a, const void *b) {
	return *((float *)a) - *((float *)b);
}

float sampleMedian(float *data, vec2 *idxs, size_t numidxs, size_t axis, size_t numsamples) {
	srand(0);
	float samples[numsamples];
	size_t randtemp;
	for (size_t i = 0; i < numsamples; i++) {
		randtemp = rand() % numidxs;
		samples[i] = data[axis ? idxs[randtemp].lon : idxs[randtemp].lat];
	}
	qsort(&samples[0], numsamples, sizeof(float), fcompare);
	return samples[numsamples / 2];
}

node *create2dTree(float *latd, float *lond, vec2 *idxs, size_t numidxs, size_t depth) {
	printf("depth %zu, numidxs %zu\n", depth, numidxs);
	node *result = (node *)malloc(sizeof(node));
	if (numidxs == 1) {
		result->location = 0.f;
		result->left = NULL;
		result->right = NULL;
		result->idx = idxs[0];
		free(idxs);
		return result;
	}
	result->location = sampleMedian(depth % 2 ? lond : latd, 
					idxs,
					numidxs,
					depth % 2,
					NUM_MEDIAN_SAMPLES < numidxs ? NUM_MEDIAN_SAMPLES : numidxs);
	printf("median %f\n", result->location);
	// these buffer sizes assume total even distrubtion, need to dynamically resize
	size_t leftidxslen = 0;
	size_t leftidxssize = numidxs / 2;
	vec2 *leftidxs = (vec2 *)malloc(leftidxssize * sizeof(vec2));
	size_t rightidxslen = 0;
	size_t rightidxssize = numidxs / 2;
	vec2 *rightidxs = (vec2 *)malloc(rightidxssize * sizeof(vec2));
	float *cmpfield = depth % 2 ? lond : latd;		
	printf("abt to make new idxs\n");
	for (size_t i = 0; i < numidxs; i++) {
		if (cmpfield[idxs[i].lat * 1354 + idxs[i].lon] < result->location) {
			//printf("left %zu\n", i);
			if (leftidxslen == leftidxssize) {
				leftidxssize *= 2;
				leftidxs = realloc(leftidxs, leftidxssize * sizeof(vec2));
			}
			leftidxs[leftidxslen] = idxs[i];
			leftidxslen++;
		}
		else {
			//printf("right %zu\n", i);
			if (rightidxslen == rightidxssize) {
				rightidxssize *= 2;
				rightidxs = realloc(rightidxs, rightidxssize * sizeof(vec2));
			}
			rightidxs[rightidxslen] = idxs[i];
			rightidxslen++;
		}
	}
	free(idxs);
	printf("abt to call new trees\n");
	result->left = create2dTree(latd, lond, leftidxs, leftidxslen, depth + 1);
	result->right = create2dTree(latd, lond, rightidxs, rightidxslen, depth + 1);
	return result;
}

void findClosestLatLonIdx(int grpid, int llids[2], float llvals[2], size_t *llidxs) {
	size_t mindist = -1u, tempidxs[2];
	float lltemp[2], disttemp;
	// below values are hard-coded, use macros later
	for (tempidxs[0] = 0; tempidxs[0] < 2030; tempidxs[0]++) {
		for (tempidxs[1] = 0; tempidxs[1] < 1354; tempidxs[1]++) {
			nc_get_var1(grpid, llids[0], &tempidxs[0], &lltemp[0]);
			nc_get_var1(grpid, llids[1], &tempidxs[0],  &lltemp[1]);
			// no sqrt because everything is relative
			disttemp = pow(lltemp[0] - llvals[0], 2) + pow(lltemp[1] - llvals[1], 2);
			if (disttemp < mindist) {
				mindist = disttemp;
				llidxs[0] = tempidxs[0];
				llidxs[1] = tempidxs[1];
			}
		}
	}
}

// arg 1: netcdf file
// arg 2: seabass file
int main(int argc, char **argv) {
	int fileid = -1, navgroupid = -1, geophysgroupid = -1, latvarid = -1, longvarid = -1;
	nc_open(argv[1], NC_NOWRITE, &fileid);
	nc_inq_ncid(fileid, "navigation_data", &navgroupid);
	nc_inq_ncid(fileid, "geophysical_data", &geophysgroupid);
	printf("nav/geophys => %i/%i\n", navgroupid, geophysgroupid);
	nc_inq_varid(navgroupid, "latitude", &latvarid);
	nc_inq_varid(navgroupid, "longitude", &longvarid);
	printf("lat/lon => %i/%i\n", latvarid, longvarid);
	
	float *latdata = (float *)malloc(2030 * 1354 * sizeof(float));
	float *londata = (float *)malloc(2030 * 1354 * sizeof(float));
	nc_get_var(navgroupid, latvarid, latdata);
	nc_get_var(navgroupid, longvarid, londata);

	vec2 *initidxs = (vec2 *)malloc(2030 * 1354 * sizeof(vec2));
	//size_t tempidxs[2];
	for (size_t x = 0; x < 2030; x++) {
		for (size_t y = 0; y < 1354; y++) {
			//tempidxs[0] = x;
			//tempidxs[1] = y;
			//nc_get_var1(navgroupid, latvarid, tempidxs, &latdata[x * 1354 + y]);
			//nc_get_var1(navgroupid, longvarid, tempidxs, &londata[x * 1354 + y]);
			initidxs[x * 1354 + y].lat = x;
			initidxs[x * 1354 + y].lon = y;
		}
	}

	create2dTree(latdata, londata, initidxs, 2030 * 1354, 0);

	size_t idxs[2];
	int latlonids[2] = {latvarid, longvarid};
	float searchvals[2] = {-20.f, 25.f};
	findClosestLatLonIdx(navgroupid, latlonids, searchvals, &idxs[0]); 
	printf("ll: %zu, %zu\n", idxs[0], idxs[1]);
	nc_close(fileid);
	return 0;
}
