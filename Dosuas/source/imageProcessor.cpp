#include "stdafx.h"
#include "imageProcessor.h"


pcl::PointCloud<pcl::PointXYZ>::Ptr ImageProcessor::filterPointCloud(pcl::PointCloud<pcl::PointXYZ>::Ptr pCloud) {
	/* removes error pixels (outliners), floor and ceiling from pointcloud */
	// TODO: improve algorithm (current is very naive)

	pcl::PointCloud<pcl::PointXYZ>::Ptr pVerticallyFilteredCloud (new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr pCompletelyFilteredCloud (new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PassThrough<pcl::PointXYZ> pass;

	// remove floor and ceiling by height filtering
	pass.setInputCloud(pCloud);
	pass.setFilterFieldName("y");
	pass.setFilterLimits(0.0, 180.0);
	pass.filter(*pVerticallyFilteredCloud);

	// remove measurement errors (outliners) by depth filtering
	/*pass.setInputCloud(pVerticallyFilteredCloud);
	pass.setFilterFieldName("z");
	pass.setFilterLimits(80.0, 530.0);
	pass.filter(*pCompletelyFilteredCloud);*/
	
	return pVerticallyFilteredCloud;
}


pcl::PointCloud<pcl::PointXYZ>::Ptr ImageProcessor::downSamplePointCloud(pcl::PointCloud<pcl::PointXYZ>::Ptr pCloud) {
	/* reduces the number of points in the pointcloud for (reduces resolution) */
	
	pcl::PointCloud<pcl::PointXYZ> cloudFiltered;
	float leaf = 2.75;  // calibrate for resolution change

	pcl::VoxelGrid<pcl::PointXYZ> vg;
	vg.setInputCloud(pCloud);
	vg.setLeafSize(leaf, leaf, leaf); 
	vg.filter(cloudFiltered);
	return cloudFiltered.makeShared();
}


std::array<int, 320 * 240> ImageProcessor::pcToImgArray(pcl::PointCloud<pcl::PointXYZ>::Ptr pCloud) {
	std::array<int, 320 * 240> imgMat;
	for (int i = 0; i < imgMat.size(); i++) {
		imgMat[i] = 0;
	}
	for (int i = 0; i < pCloud->size(); i++) {
		imgMat[pCloud->points[i].y * 320 + pCloud->points[i].x] = pCloud->points[i].z;
	}
	return imgMat;
}


Voxel ImageProcessor::getNearestVoxel(std::array<pcl::PointXYZ, 240> column) {
	/* gets a single voxel (nearest point), wich represents the column, which is heared */
	// TODO: filtering 
	int i = 0, j = 0, sum = 0, num = 0;
	Voxel vxl;
	std::sort(column.begin(), column.end(), [](pcl::PointXYZ a, pcl::PointXYZ b) {return a.z > b.z;});
	while (column.at(i).z == 0) { i++; }  // skip invalid voxels
	for (j = i; (j < i + 10) && (j < 240); j++) {
		sum += column.at(j).z;
		num++;
	}  // throws error if all pixels are invalid
	vxl.x = column.at(j).x;
	vxl.y = 1;  // TODO: later add y to sounds
	vxl.z = sum / num;  
	return vxl;
}


void ImageProcessor::showPointCloud(pcl::PointCloud<pcl::PointXYZ>::Ptr pCloud) {
	/* displays pointcloud in simple 3D viewer */
	
	pcl::visualization::CloudViewer viewer("Simple Cloud Viewer");
	viewer.showCloud(pCloud);
	while (!viewer.wasStopped()) {}
}


std::vector<Voxel> ImageProcessor::getVoxelsForAudioSwipe(pcl::PointCloud<pcl::PointXYZ>::Ptr pCloud) {
	/* calculates voxels, which are played in the audio swipe; takes image buffer (filtered pointcloud in 
	2d matrix form) as input */

	//showPointCloud(pCloud);
	pcl::PointCloud<pcl::PointXYZ>::Ptr pFilteredCloud = filterPointCloud(pCloud);
	//showPointCloud(pFilteredCloud);
	//pcl::PointCloud<pcl::PointXYZ>::Ptr pDownsampledCloud = downSamplePointCloud(pFilteredCloud);
	//showPointCloud(pDownsampledCloud);
	std::array<int, 320 * 240> imgMat = pcToImgArray(pCloud);
	std::vector<Voxel> voxels;
	for (int i = 0; i < 320; i++) {
		std::array<pcl::PointXYZ, 240> col;
		for (int j = 0; j < 240; j++) {
			pcl::PointXYZ point;
			point.x = i;
			point.y = j;
			point.z = imgMat.at(i + j * 320);
			col.at(j) = point;
		}
		Voxel vxl = getNearestVoxel(col);
		//std::printf("Voxel coordinates: %i %i %i\n", vxl.x, vxl.y, vxl.z);
		voxels.push_back(vxl);
	}
	return voxels;
}


std::vector<std::vector<int>> ImageProcessor::getImageForChordSwipe(pcl::PointCloud<pcl::PointXYZ>::Ptr pCloud) {
	std::array<int, 320 * 240> imgArray = pcToImgArray(pCloud);
	std::vector<std::vector<int>> columns;
	int depthSum;
	for (int i = 0; i < 320; i++) {
		depthSum = 0;
		std::vector<int> column;
		for (int j = 0; j < 240; j++) {
			if ((j + 1) % 10 == 0) {
				column.push_back((float)depthSum / 10.0f);
				//std::cout << "depth sum: " << (float)depthSum / 10.0f << std::endl;
				depthSum = 0;
			}
			depthSum += imgArray.at(i + j * 320);
		}
		columns.push_back(column);
	}
	return columns;
}