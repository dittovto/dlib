// Copyright (C) 2012  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_LOAD_IMAGE_DaTASET_Hh_
#define DLIB_LOAD_IMAGE_DaTASET_Hh_

#include "load_image_dataset_abstract.h"
#include "../misc_api.h"
#include "../dir_nav.h"
#include "../image_io.h"
#include "../array.h"
#include <vector>
#include "../geometry.h"
#include "image_dataset_metadata.h"
#include <string>
#include <set>
#include "../image_processing/full_object_detection.h"
#include <utility>
#include <limits>
#include "../image_transforms/image_pyramid.h"
#include "../geometry/rectangle.h"
#include <random>

namespace dlib
{

// ----------------------------------------------------------------------------------------

    class image_dataset_file
    {
    public:
        image_dataset_file(const std::string& filename)
        {
            _skip_empty_images = false;
            _have_parts = false;
            _filename = filename;
            _box_area_thresh = std::numeric_limits<double>::infinity();
        }

        image_dataset_file boxes_match_label(
            const std::string& label
        ) const
        {
            image_dataset_file temp(*this);
            temp._labels.insert(label);
            return temp;
        }

        image_dataset_file skip_empty_images(
        ) const
        {
            image_dataset_file temp(*this);
            temp._skip_empty_images = true;
            return temp;
        }

        image_dataset_file boxes_have_parts(
        ) const
        {
            image_dataset_file temp(*this);
            temp._have_parts = true;
            return temp;
        }

        image_dataset_file shrink_big_images(
            double new_box_area_thresh = 150*150
        ) const
        {
            image_dataset_file temp(*this);
            temp._box_area_thresh = new_box_area_thresh;
            return temp;
        }

        bool should_load_box (
            const image_dataset_metadata::box& box
        ) const
        {
            if (_have_parts && box.parts.size() == 0)
                return false;
            if (_labels.size() == 0)
                return true;
            if (_labels.count(box.label) != 0)
                return true;
            return false;
        }

        const std::string& get_filename() const { return _filename; }
        bool should_skip_empty_images() const { return _skip_empty_images; }
        bool should_boxes_have_parts() const { return _have_parts; }
        double box_area_thresh() const { return _box_area_thresh; }
        const std::set<std::string>& get_selected_box_labels() const { return _labels; }

    private:
        std::string _filename;
        std::set<std::string> _labels;
        bool _skip_empty_images;
        bool _have_parts;
        double _box_area_thresh;

    };

// ----------------------------------------------------------------------------------------

    template <
        typename array_type
        >
    std::vector<std::vector<rectangle> > load_image_dataset (
        array_type& images,
        std::vector<std::vector<rectangle> >& object_locations,
        const image_dataset_file& source
    )
    {
        images.clear();
        object_locations.clear();

        std::vector<std::vector<rectangle> > ignored_rects;

        using namespace dlib::image_dataset_metadata;
        dataset data;
        load_image_dataset_metadata(data, source.get_filename());

        // Set the current directory to be the one that contains the
        // metadata file. We do this because the file might contain
        // file paths which are relative to this folder.
        locally_change_current_dir chdir(get_parent_directory(file(source.get_filename())));


        typedef typename array_type::value_type image_type;


        image_type img;
        std::vector<rectangle> rects, ignored;
        for (unsigned long i = 0; i < data.images.size(); ++i)
        {
            double min_rect_size = std::numeric_limits<double>::infinity();
            rects.clear();
            ignored.clear();
            for (unsigned long j = 0; j < data.images[i].boxes.size(); ++j)
            {
                if (source.should_load_box(data.images[i].boxes[j]))
                {
                    if (data.images[i].boxes[j].ignore)
                    {
                        ignored.push_back(data.images[i].boxes[j].rect);
                    }
                    else
                    {
                        rects.push_back(data.images[i].boxes[j].rect);
                        min_rect_size = std::min<double>(min_rect_size, rects.back().area());
                    }
                }
            }

            if (!source.should_skip_empty_images() || rects.size() != 0)
            {
                load_image(img, data.images[i].filename);
                if (rects.size() != 0)
                {
                    // if shrinking the image would still result in the smallest box being
                    // bigger than the box area threshold then shrink the image.
                    while(min_rect_size/2/2 > source.box_area_thresh())
                    {
                        pyramid_down<2> pyr;
                        pyr(img);
                        min_rect_size *= (1.0/2.0)*(1.0/2.0);
                        for (auto&& r : rects)
                            r = pyr.rect_down(r);
                        for (auto&& r : ignored)
                            r = pyr.rect_down(r);
                    }
                    while(min_rect_size*(2.0/3.0)*(2.0/3.0) > source.box_area_thresh())
                    {
                        pyramid_down<3> pyr;
                        pyr(img);
                        min_rect_size *= (2.0/3.0)*(2.0/3.0);
                        for (auto&& r : rects)
                            r = pyr.rect_down(r);
                        for (auto&& r : ignored)
                            r = pyr.rect_down(r);
                    }
                }
                images.push_back(img);
                object_locations.push_back(rects);
                ignored_rects.push_back(ignored);
            }
        }

        return ignored_rects;
    }

// ----------------------------------------------------------------------------------------

    template <
        typename array_type
        >
    void load_image_dataset (
        array_type& images,
        std::vector<std::vector<mmod_rect> >& object_locations,
        const image_dataset_file& source
    )
    {
        images.clear();
        object_locations.clear();

        using namespace dlib::image_dataset_metadata;
        dataset data;
        load_image_dataset_metadata(data, source.get_filename());

        // Set the current directory to be the one that contains the
        // metadata file. We do this because the file might contain
        // file paths which are relative to this folder.
        locally_change_current_dir chdir(get_parent_directory(file(source.get_filename())));

        typedef typename array_type::value_type image_type;

        image_type img;
        std::vector<mmod_rect> rects;
        for (unsigned long i = 0; i < data.images.size(); ++i)
        {
            double min_rect_size = std::numeric_limits<double>::infinity();
            rects.clear();
            for (unsigned long j = 0; j < data.images[i].boxes.size(); ++j)
            {
                if (source.should_load_box(data.images[i].boxes[j]))
                {
                    if (data.images[i].boxes[j].ignore)
                    {
                        rects.push_back(ignored_mmod_rect(data.images[i].boxes[j].rect));
                    }
                    else
                    {
                        rects.push_back(mmod_rect(data.images[i].boxes[j].rect));
                        min_rect_size = std::min<double>(min_rect_size, rects.back().rect.area());
                    }
                    rects.back().label = data.images[i].boxes[j].label;

                }
            }

            if (!source.should_skip_empty_images() || rects.size() != 0)
            {
                load_image(img, data.images[i].filename);
                if (rects.size() != 0)
                {
                    // if shrinking the image would still result in the smallest box being
                    // bigger than the box area threshold then shrink the image.
                    while(min_rect_size/2/2 > source.box_area_thresh())
                    {
                        pyramid_down<2> pyr;
                        pyr(img);
                        min_rect_size *= (1.0/2.0)*(1.0/2.0);
                        for (auto&& r : rects)
                            r.rect = pyr.rect_down(r.rect);
                    }
                    while(min_rect_size*(2.0/3.0)*(2.0/3.0) > source.box_area_thresh())
                    {
                        pyramid_down<3> pyr;
                        pyr(img);
                        min_rect_size *= (2.0/3.0)*(2.0/3.0);
                        for (auto&& r : rects)
                            r.rect = pyr.rect_down(r.rect);
                    }
                }
                images.push_back(std::move(img));
                object_locations.push_back(std::move(rects));
            }
        }
    }

// ----------------------------------------------------------------------------------------

// ******* THIS FUNCTION IS DEPRECATED, you should use another version of load_image_dataset() *******
    template <
        typename image_type,
        typename MM
        >
    std::vector<std::vector<rectangle> > load_image_dataset (
        array<image_type,MM>& images,
        std::vector<std::vector<rectangle> >& object_locations,
        const std::string& filename,
        const std::string& label,
        bool skip_empty_images = false
    )
    {
        image_dataset_file f(filename);
        if (label.size() != 0)
            f = f.boxes_match_label(label);
        if (skip_empty_images)
            f = f.skip_empty_images();
        return load_image_dataset(images, object_locations, f);
    }

// ----------------------------------------------------------------------------------------

    template <
        typename array_type
        >
    std::vector<std::vector<rectangle> > load_image_dataset (
        array_type& images,
        std::vector<std::vector<rectangle> >& object_locations,
        const std::string& filename
    )
    {
        return load_image_dataset(images, object_locations, image_dataset_file(filename));
    }

// ----------------------------------------------------------------------------------------

    template <
        typename array_type
        >
    void load_image_dataset (
        array_type& images,
        std::vector<std::vector<mmod_rect>>& object_locations,
        const std::string& filename
    )
    {
        load_image_dataset(images, object_locations, image_dataset_file(filename));
    }

// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

    void rotate_point(const point& ctr,float angle, point& p)
    {
      float s = std::sin(angle);
      float c = std::cos(angle);

      // translate point back to origin:
      p.x() -= ctr.x();
      p.y() -= ctr.y();

      // rotate point
      float xnew = p.x() * c - p.y() * s;
      float ynew = p.x() * s + p.y() * c;

      // translate point back:
      p.x() = xnew + ctr.x();
      p.y() = ynew + ctr.y();
    }

    template <
        typename array_type
        >
    std::vector<std::vector<rectangle> > load_image_dataset (
        array_type& images,
        std::vector<std::vector<full_object_detection> >& object_locations,
        const image_dataset_file& source,
        std::vector<std::string>& parts_list,
        const std::vector<double>& augment_rotations
    )
    {
        std::cout << "Entering load_image_dataset" << std::endl;
        typedef typename array_type::value_type image_type;
        parts_list.clear();
        images.clear();
        object_locations.clear();
        assert(augment_rotations.size() == 2 || augment_rotations.size() == 0);
        int aug_num_samples = 0;
        double aug_max_angle = 0;
        std::vector<double> aug_rotation_angles;
        if (augment_rotations.size()) {
            aug_num_samples = int(augment_rotations[0]);
            aug_max_angle = augment_rotations[1] * 3.141592657 / 180.;
            aug_rotation_angles = std::vector<double>(aug_num_samples * 2, 0);
        }

        if (aug_num_samples) {
            double slice_angle = aug_max_angle / aug_num_samples;
            for (size_t slice_idx = 0; slice_idx < aug_num_samples; ++slice_idx ) {
                aug_rotation_angles[slice_idx] = -slice_angle * (aug_num_samples - slice_idx);
            }
            for (size_t slice_idx = 0; slice_idx < aug_num_samples; ++slice_idx ) {
                aug_rotation_angles[aug_num_samples + slice_idx] = slice_angle * (slice_idx + 1);
            }
            std::cout << "Sampling Angles: ";
            for ( const auto & angle : aug_rotation_angles ) {
                std::cout << angle * 180. / 3.141592657 << ", ";
            }
            std::cout << std::endl;
        }


        using namespace dlib::image_dataset_metadata;
        dataset data;
        load_image_dataset_metadata(data, source.get_filename());

        // Set the current directory to be the one that contains the
        // metadata file. We do this because the file might contain
        // file paths which are relative to this folder.
        locally_change_current_dir chdir(get_parent_directory(file(source.get_filename())));


        std::set<std::string> all_parts;

        // find out what parts are being used in the dataset.  Store results in all_parts.
        for (unsigned long i = 0; i < data.images.size(); ++i)
        {
            for (unsigned long j = 0; j < data.images[i].boxes.size(); ++j)
            {
                if (source.should_load_box(data.images[i].boxes[j]))
                {
                    const std::map<std::string,point>& parts = data.images[i].boxes[j].parts;
                    std::map<std::string,point>::const_iterator itr;

                    for (itr = parts.begin(); itr != parts.end(); ++itr)
                    {
                        all_parts.insert(itr->first);
                    }
                }
            }
        }

        // make a mapping between part names and the integers [0, all_parts.size())
        std::map<std::string,int> parts_idx;
        for (std::set<std::string>::iterator i = all_parts.begin(); i != all_parts.end(); ++i)
        {
            parts_idx[*i] = parts_list.size();
            parts_list.push_back(*i);
        }

        std::vector<std::vector<rectangle> > ignored_rects;
        std::vector<rectangle> ignored;
        image_type img, original_img, rotated_image;
        image_type template_img;
        std::vector<full_object_detection> object_dets;
        load_image(template_img, data.images[0].filename);

        for (unsigned long i = 0; i < data.images.size(); ++i)
        {
            double min_rect_size = std::numeric_limits<double>::infinity();
            object_dets.clear();
            ignored.clear();
            assert(data.images[i].boxes.size() == 1);
            for (unsigned long j = 0; j < data.images[i].boxes.size(); ++j)
            {
                if (source.should_load_box(data.images[i].boxes[j]))
                {
                    if (data.images[i].boxes[j].ignore)
                    {
                        ignored.push_back(data.images[i].boxes[j].rect);
                    }
                    else
                    {
                        std::vector<point> partlist(parts_idx.size(), OBJECT_PART_NOT_PRESENT);

                        // populate partlist with all the parts present in this box.
                        const std::map<std::string,point>& parts = data.images[i].boxes[j].parts;
                        std::map<std::string,point>::const_iterator itr;
                        for (itr = parts.begin(); itr != parts.end(); ++itr)
                        {
                            partlist[parts_idx[itr->first]] = itr->second;
                        }

                        object_dets.clear();
                        object_dets.push_back(full_object_detection(data.images[i].boxes[j].rect, partlist));
                        min_rect_size = std::min<double>(min_rect_size, object_dets.back().get_rect().area());
                        object_locations.push_back(object_dets);


                        if (aug_rotation_angles.size()) {
                            const auto& rect = data.images[i].boxes[j].rect;
                            point ctr = dcenter(get_rect(template_img));
                            for (const auto & angle : aug_rotation_angles) {
                                object_dets.clear();
                                std::vector<point> new_partlist(partlist);

                                for (itr = parts.begin(); itr != parts.end(); ++itr)
                                {
                                    rotate_point(ctr, angle, new_partlist[parts_idx[itr->first]]);
                                }
                                object_dets.push_back(full_object_detection(rect, new_partlist));
                                object_locations.push_back(object_dets);
                            }
                        }
                    }
                }
            }

            if (!source.should_skip_empty_images() || object_dets.size() != 0)
            {
                load_image(img, data.images[i].filename);
                load_image(original_img, data.images[i].filename);
                if (object_dets.size() != 0)
                {
                    double original_min_rect_size = min_rect_size;
                    // if shrinking the image would still result in the smallest box being
                    // bigger than the box area threshold then shrink the image.
                    while(min_rect_size/2/2 > source.box_area_thresh())
                    {
                        pyramid_down<2> pyr;
                        pyr(img);
                        min_rect_size *= (1.0/2.0)*(1.0/2.0);
                        for (auto&& r : object_dets)
                        {
                            r.get_rect() = pyr.rect_down(r.get_rect());
                            for (unsigned long k = 0; k < r.num_parts(); ++k)
                                r.part(k) = pyr.point_down(r.part(k));
                        }
                        for (auto&& r : ignored)
                        {
                            r = pyr.rect_down(r);
                        }
                    }
                    while(min_rect_size*(2.0/3.0)*(2.0/3.0) > source.box_area_thresh())
                    {
                        pyramid_down<3> pyr;
                        pyr(img);
                        min_rect_size *= (2.0/3.0)*(2.0/3.0);
                        for (auto&& r : object_dets)
                        {
                            r.get_rect() = pyr.rect_down(r.get_rect());
                            for (unsigned long k = 0; k < r.num_parts(); ++k)
                                r.part(k) = pyr.point_down(r.part(k));
                        }
                        for (auto&& r : ignored)
                        {
                            r = pyr.rect_down(r);
                        }
                    }
                    images.push_back(img);
                    if (aug_rotation_angles.size()) {
                        for (const auto & angle : aug_rotation_angles) {
                            rotate_image_inplace(original_img, rotated_image, -angle);
                            min_rect_size = original_min_rect_size;
                            // if shrinking the image would still result in the smallest box being
                            // bigger than the box area threshold then shrink the image.
                            while(min_rect_size/2/2 > source.box_area_thresh())
                            {
                                pyramid_down<2> pyr;
                                pyr(rotated_image);
                                min_rect_size *= (1.0/2.0)*(1.0/2.0);
                            }
                            while(min_rect_size*(2.0/3.0)*(2.0/3.0) > source.box_area_thresh())
                            {
                                pyramid_down<3> pyr;
                                pyr(rotated_image);
                                min_rect_size *= (2.0/3.0)*(2.0/3.0);
                            }
                            images.push_back(rotated_image);
                        }
                    }
                }
                ignored_rects.push_back(ignored);


            }
        }


        return ignored_rects;
    }

// ----------------------------------------------------------------------------------------

    template <
        typename array_type
        >
    std::vector<std::vector<rectangle> > load_image_dataset (
        array_type& images,
        std::vector<std::vector<full_object_detection> >& object_locations,
        const image_dataset_file& source,
        const std::vector<double>& augment_rotations
    )
    {
        std::vector<std::string> parts_list;
        return load_image_dataset(images, object_locations, source, parts_list, augment_rotations);
    }

// ----------------------------------------------------------------------------------------

    template <
        typename array_type
        >
    std::vector<std::vector<rectangle> > load_image_dataset (
        array_type& images,
        std::vector<std::vector<full_object_detection> >& object_locations,
        const std::string& filename,
        const std::vector<double>& augment_rotations
    )
    {
        std::vector<std::string> parts_list;
        return load_image_dataset(images, object_locations, image_dataset_file(filename), parts_list, augment_rotations);
    }


    template <
        typename array_type
        >
    std::vector<std::vector<rectangle> > load_image_dataset (
        array_type& images,
        std::vector<std::vector<full_object_detection> >& object_locations,
        const std::string& filename
    )
    {
        std::vector<std::string> parts_list;
        std::vector<double> augment_rotations;
        return load_image_dataset(images, object_locations, image_dataset_file(filename), parts_list, augment_rotations);
    }


// ----------------------------------------------------------------------------------------

}

#endif // DLIB_LOAD_IMAGE_DaTASET_Hh_
