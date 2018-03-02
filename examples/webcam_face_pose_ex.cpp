// The contents of this file are in the public domain. See
// LICENSE_FOR_EXAMPLE_PROGRAMS.txt
/*

    This example program shows how to find frontal human faces in an image and
    estimate their pose.  The pose takes the form of 68 landmarks.  These are
    points on the face such as the corners of the mouth, along the eyebrows, on
    the eyes, and so forth.


    This example is essentially just a version of the
   face_landmark_detection_ex.cpp
    example modified to use OpenCV's VideoCapture object to read from a camera
   instead
    of files.


    Finally, note that the face detector is fastest when compiled with at least
    SSE2 instructions enabled.  So if you are using a PC with an Intel or AMD
    chip then you should enable at least SSE2 instructions.  If you are using
    cmake to compile this program you can enable them by using one of the
    following commands when you create the build project:
        cmake path_to_dlib_root/examples -DUSE_SSE2_INSTRUCTIONS=ON
        cmake path_to_dlib_root/examples -DUSE_SSE4_INSTRUCTIONS=ON
        cmake path_to_dlib_root/examples -DUSE_AVX_INSTRUCTIONS=ON
    This will set the appropriate compiler options for GCC, clang, Visual
    Studio, or the Intel compiler.  If you are using another compiler then you
    need to consult your compiler's manual to determine how to enable these
    instructions.  Note that AVX is the fastest but requires a CPU from at least
    2011.  SSE4 is the next fastest and is supported by most current machines.
*/

#include <dlib/opencv.h>
#include <opencv2/highgui/highgui.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>

using namespace dlib;
using namespace std;

rectangle make_square(const rectangle& face) {
  float center_x = (face.left() + face.right()) / 2.;
  float center_y = (face.top() + face.bottom()) / 2.;
  float width = face.right() - face.left();
  float height = face.bottom() - face.top();
  float avg_side = (width + height) / 2.;
  float avg_half_side = avg_side / 2.;

  float left = int(center_x - avg_half_side + 0.5);
  float right = int(center_x + avg_half_side + 0.5);
  float top = int(center_y - avg_half_side + 0.5);
  float bottom = int(center_y + avg_half_side + 0.5);
  return rectangle(left, top, right, bottom);
}

int main(int argc, char** argv) {
  try {
    int camera = stoi(argc > 3 ? argv[3] : 0);
    cv::VideoCapture cap(camera);
    if (!cap.isOpened()) {
      cerr << "Unable to connect to camera" << endl;
      return 1;
    }

    image_window win;

    // Load face detection and pose estimation models.
    frontal_face_detector detector = get_frontal_face_detector();
    shape_predictor pose_model, left_pose_model;
    deserialize(argv[1]) >> pose_model;
    deserialize(argv[2]) >> left_pose_model;

    correlation_tracker tracker;
    array2d<unsigned char> last_img;
    rectangle last_face;
    bool tracking = false;
    int face_detector_index = -1;

    // Grab and process frames until the main window is closed by the user.
    while (!win.is_closed()) {
      // Grab a frame
      cv::Mat frame, rev_frame;
      if (!cap.read(frame)) {
        break;
      }
      int width = frame.cols, height = frame.rows;

      // Turn OpenCV's Mat into something dlib can deal with.  Note that this
      // just
      // wraps the Mat object, it doesn't copy anything.  So cimg is only valid
      // as
      // long as frame is valid.  Also don't do anything to frame that would
      // cause it
      // to reallocate the memory which stores the image as that will make cimg
      // contain dangling pointers.  This basically means you shouldn't modify
      // frame
      // while using cimg.
      cv_image<bgr_pixel> cimg(frame);

      // Detect faces
      std::vector<rect_detection> face_detections;
      detector(cimg, face_detections);
      rect_detection face_detection;
      rectangle face;
      int use_detector;

      if (face_detections.size()) {
        face_detection = face_detections[0];
        face = face_detection.rect;
        tracking = false;
        face_detector_index = face_detection.weight_index;
        double face_score = face_detection.detection_confidence;
        cout << "Face Index: " << face_detector_index;
        cout << ", Score: " << face_score;
        cout << ", BBox Size: (" << face.width() << ", " << face.height() << ")" << std::endl;
        if (face_score < 2.0) {
          use_detector = face_detector_index;
        } else {
          use_detector = 0;
        }
      } else {
        if (last_face.is_empty()) {
          continue;
        }
        if (!tracking) {
          tracker.start_track(last_img, last_face);
        }
        tracker.update(cimg);
        face = make_square(tracker.get_position());
        tracking = true;
        use_detector = face_detector_index;
      }
      win.clear_overlay();
      win.set_image(cimg);
      win.add_overlay(face);

      size_t face_w = face.width(), face_h = face.height();

      if (use_detector == 0) {
        // Display it all on the screen
        std::vector<full_object_detection> full_faces;
        // std::vector<double> scales = {0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4};
        std::vector<double> scales = {1.0};
        for (double scale : scales) {
          rectangle scaled_face(centered_rect(face, face_w * scale, face_h * scale));
          win.add_overlay(scaled_face);
          full_faces.push_back(pose_model(cimg, scaled_face));
        }
        win.add_overlay(
            render_face_detections(full_faces, rgb_pixel(255, 0, 0)));
      } else if (use_detector == 2) {
        // Find the left landmarks.
        std::vector<full_object_detection> left_faces;
        std::vector<double> scales = {1.0};
        // std::vector<double> scales = {0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4};
        for (double scale : scales) {
          rectangle scaled_face(centered_rect(face, face_w * scale, face_h * scale));
          win.add_overlay(scaled_face);
          left_faces.push_back(left_pose_model(cimg, scaled_face));
        }
        win.add_overlay(render_half_face_detections(left_faces));
      } else if (use_detector == 1) {
        rev_frame = frame.clone();
        cv::flip(rev_frame, rev_frame, 1);
        cv_image<bgr_pixel> rev_cimg(rev_frame);

        // For reverse side.
        rectangle rev_face(width - face.right() - 1, face.top(),
                           width - face.left() - 1, face.bottom());

        std::vector<full_object_detection> right_faces;
        full_object_detection rev_pose = left_pose_model(rev_cimg, rev_face);
        for (int i = 0; i < rev_pose.num_parts(); ++i) {
          rev_pose.part(i).x() = width - 1 - rev_pose.part(i).x();
        }

        right_faces.push_back(rev_pose);
        win.add_overlay(render_half_face_detections(right_faces));
      }

      assign_image(last_img, cimg);
      last_face = face;
    }
  } catch (serialization_error& e) {
    cout << "You need dlib's default face landmarking model file to run this "
            "example." << endl;
    cout << "You can get it from the following URL: " << endl;
    cout << "   http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2"
         << endl;
    cout << endl
         << e.what() << endl;
  } catch (exception& e) {
    cout << e.what() << endl;
  }
}
