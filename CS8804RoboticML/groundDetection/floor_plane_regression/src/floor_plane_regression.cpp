#include <ros/ros.h>
#include <visualization_msgs/Marker.h>
#include <sensor_msgs/PointCloud2.h>
#include <pcl_ros/point_cloud.h>
#include <pcl_ros/transforms.h>
#include <pcl/point_types.h>
#include <tf/tf.h>
#include <tf/transform_listener.h>
#include <sys/time.h>

#include <Eigen/Core>
#include <Eigen/Cholesky>

class FloorPlaneRegression {
    protected:
        ros::Subscriber scan_sub_;
        ros::Publisher marker_pub_;
        tf::TransformListener listener_;
	
        ros::NodeHandle nh_;
        std::string base_frame_;
        double max_range_;

        pcl::PointCloud<pcl::PointXYZ> lastpc_;

    protected: // ROS Callbacks

        void pc_callback(const sensor_msgs::PointCloud2ConstPtr msg) {
			
			// Timer initialization
			struct timeval tbegin,tend;
			double texec=0.;
			gettimeofday(&tbegin,NULL);

            // Receive the point cloud and convert it to the right format
            pcl::PointCloud<pcl::PointXYZ> temp;
            pcl::fromROSMsg(*msg, temp);
            // Projects the point cloud into the base-frame
            listener_.waitForTransform(base_frame_,msg->header.frame_id,msg->header.stamp,ros::Duration(1.0));
            pcl_ros::transformPointCloud(base_frame_,msg->header.stamp, temp, msg->header.frame_id, lastpc_, listener_);

            unsigned int n = temp.size();
            std::vector<size_t> pidx; // Indices of useful points i.e. points 
            // that are neither too near nor too far
            for (unsigned int i=0;i<n;i++) {
                float x = temp[i].x;
                float y = temp[i].y;
                float d = hypot(x,y);
                // In the sensor frame, this point would be inside the camera
                if (d < 1e-2) {
                    // Bogus point, ignore
                    continue;
                }
                // Measure the point distance in the base frame
                x = lastpc_[i].x;
                y = lastpc_[i].y;
                d = hypot(x,y);
                if (d > max_range_) {
                    // too far, ignore
                    continue;
                }
                // If we reach this stage, we have an acceptable point, so
                // let's store its index
                pidx.push_back(i);
            }
            
            // TODO START
            // Linear regression: z = a*x + b*y + c
            // The code below use Eigen to find the parameters of the
            // linear regression above. 
            //
            // n: number of useful point in the point cloud
            n = pidx.size();
            Eigen::MatrixXf A(n,3);
            Eigen::MatrixXf B(n,1);
            for (unsigned int i=0;i<n;i++) {
                // Assign x,y,z to the coordinates of the point we are
                // considering.
                double x = lastpc_[pidx[i]].x;
                double y = lastpc_[pidx[i]].y;
                double z = lastpc_[pidx[i]].z;
                A(i,0) = x;
                A(i,1) = y;
                A(i,2) = 1;
                B(i,0) = z;
            }

            Eigen::MatrixXf X = (A.transpose() * A).inverse() * A.transpose() *B;
            // Details on linear solver can be found on 
            // http://eigen.tuxfamily.org/dox-devel/group__TutorialLinearAlgebra.html
            
            // The result is computed in vector X
            ROS_INFO("Extracted floor plane: z = %.2fx + %.2fy + %.2f",
                    X(0),X(1),X(2));

            // END OF TODO


            // Now build an orientation vector to display a marker in rviz
            // First we build a basis of the plane normal to its normal vector
            Eigen::Vector3f O,u,v,w;
            w << X(0), X(1), -1.0;
            w /= w.norm();
            O << 1.0, 0.0, 1.0*X(0)+0.0*X(1)+X(2);
            u << 2.0, 0.0, 2.0*X(0)+0.0*X(1)+X(2);
            u -= O;
            u /= u.norm();
            v = w.cross(u);
            // Then we build a rotation matrix out of it
            tf::Matrix3x3 R(u(0),v(0),w(0),
                    u(1),v(1),w(1),
                    u(2),v(2),w(2));
            // And convert it to a quaternion
            tf::Quaternion Q;
            R.getRotation(Q);
            
            // Documentation on visualization markers can be found on:
            // http://www.ros.org/wiki/rviz/DisplayTypes/Marker
            visualization_msgs::Marker m;
            m.header.stamp = msg->header.stamp;
            m.header.frame_id = base_frame_;
            m.ns = "floor_plane";
            m.id = 1;
            m.type = visualization_msgs::Marker::CYLINDER;
            m.action = visualization_msgs::Marker::ADD;
            m.pose.position.x = O(0);
            m.pose.position.y = O(1);
            m.pose.position.z = O(2);
            tf::quaternionTFToMsg(Q,m.pose.orientation);
            m.scale.x = 1.0;
            m.scale.y = 1.0;
            m.scale.z = 0.01;
            m.color.a = 0.5;
            m.color.r = 0.0;
            m.color.g = 1.0;
            m.color.b = 0.0;
            // Finally publish the marker
            marker_pub_.publish(m);
            
            //timer sample
			gettimeofday(&tend,NULL);
			texec=((double)(1000*(tend.tv_sec-tbegin.tv_sec)+((tend.tv_usec-tbegin.tv_usec)/1000)))/1000.;
			std::cout << "\n" << "time = " << texec << "\n" << std::endl ;

        }

    public:
        FloorPlaneRegression() : nh_("~") {
            // TODO START
            //These parameters are set in the launch file.
            // The parameter below described the frame in which the point cloud
            // must be projected to be estimated.
            nh_.param("base_frame",base_frame_,std::string("/body"));
            // This parameter defines the maximum range at which we want to
            // consider points. 
            nh_.param("max_range",max_range_,5.0);
            // END OF TODO

            // Make sure TF is ready
            ros::Duration(0.5).sleep();

            // Subscribe to the point cloud and prepare the marker publisher
            scan_sub_ = nh_.subscribe("scans",1,&FloorPlaneRegression::pc_callback,this);
            marker_pub_ = nh_.advertise<visualization_msgs::Marker>("floor_plane",1);
        }

};

int main(int argc, char * argv[]) 
{
	ros::init(argc,argv,"floor_plane_regression");
    FloorPlaneRegression fp;
    ros::spin();
  	return 0;
}


