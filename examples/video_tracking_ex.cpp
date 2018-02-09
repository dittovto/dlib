// The contents of this file are in the public domain. See LICENSE_FOR_EXAMPLE_PROGRAMS.txt
/*

    This example shows how to use the correlation_tracker from the dlib C++ library.  This
    object lets you track the position of an object as it moves from frame to frame in a
    video sequence.  To use it, you give the correlation_tracker the bounding box of the
    object you want to track in the current video frame.  Then it will identify the
    location of the object in subsequent frames.

    In this particular example, we are going to run on the video sequence that comes with
    dlib, which can be found in the examples/video_frames folder.  This video shows a juice
    box sitting on a table and someone is waving the camera around.  The task is to track the
    position of the juice box as the camera moves around.
*/


#include <opencv2/opencv.hpp>
#include <dlib/opencv.h>

#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <dlib/dir_nav.h>


using namespace dlib;
using namespace std;

class Timer {
  using clock = std::chrono::high_resolution_clock;

 public:
  Timer() : last_(clock::now()) {}
  void start() { last_ = clock::now(); }
  int seconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(clock::now() -
                                                            last_).count();
  }
  int milliseconds() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() -
                                                                 last_).count();
  }

 private:
  clock::time_point last_;
};

int main(int argc, char** argv) try
{
    // if (argc != 2)
    // {
    //     cout << "Call this program like this: " << endl;
    //     cout << "./video_tracking_ex ../video_frames" << endl;
    //     return 1;
    // }

    cv::VideoCapture cap(0);
    if (!cap.isOpened())
    {
        cerr << "Unable to connect to camera" << endl;
        return 1;
    }


    // // Get the list of video frames.  
    // std::vector<file> files = get_files_in_directory_tree(argv[1], match_ending(".jpg"));
    // std::sort(files.begin(), files.end());
    // if (files.size() == 0)
    // {
    //     cout << "No images found in " << argv[1] << endl;
    //     return 1;
    // }

    for (size_t i = 0; i < 10; ++i) {
        cv::Mat frame;
        cap >> frame;
    }
    frontal_face_detector detector = get_frontal_face_detector();
    shape_predictor pose_model;
    deserialize("/home/daranday/Models/dlib/shape_predictor_68_face_landmarks.dat") >> pose_model;

    // Load the first frame.  
    array2d<unsigned char> img;
    correlation_tracker tracker;
    // load_image(img, files[0]);
    {
        cv::Mat frame;
        cap >> frame;
        dlib::cv_image<dlib::bgr_pixel> cv_image(frame);
        assign_image(img, cv_image);
        std::vector<rectangle> faces = detector(cv_image);
        tracker.start_track(img, faces[0]);
    }
    // Now create a tracker and start a track on the juice box.  If you look at the first
    // frame you will see that the juice box is centered at pixel point(92,110) and 38
    // pixels wide and 86 pixels tall.

    // Now run the tracker.  All we have to do is call tracker.update() and it will keep
    // track of the juice box!
    image_window win;
    // for (unsigned long i = 1; i < files.size(); ++i)
    while (true)
    {
        cv::Mat frame;
        cap >> frame;
        dlib::cv_image<dlib::bgr_pixel> cv_image(frame);
        assign_image(img, cv_image);
        // load_image(img, files[i]);
        tracker.update(img);
        auto det = tracker.get_position();
        std::vector<full_object_detection> shapes;
        shapes.push_back(pose_model(cv_image, det));

        win.set_image(img); 
        win.clear_overlay(); 
        win.add_overlay(det);
        win.add_overlay(render_face_detections(shapes));

        // cout << "hit enter to process next frame" << endl;
        // cin.get();
    }
}
catch (std::exception& e)
{
    cout << e.what() << endl;
}

