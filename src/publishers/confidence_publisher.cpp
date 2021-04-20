/*
 * Copyright (c) 2021 Roboception GmbH
 * All rights reserved
 *
 * Author: Heiko Hirschmueller
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "confidence_publisher.hpp"

#include <rc_genicam_api/pixel_formats.h>

#include <sensor_msgs/image_encodings.hpp>

namespace rc
{

ConfidencePublisher::ConfidencePublisher(rclcpp::Node * node, const std::string & frame_id)
: GenICam2RosPublisher(frame_id)
{
  pub = image_transport::create_publisher(node, "stereo/confidence");
}

bool ConfidencePublisher::used()
{
  return pub.getNumSubscribers() > 0;
}

void ConfidencePublisher::requiresComponents(int & components, bool &)
{
  if (pub.getNumSubscribers() > 0) {
    components |= ComponentConfidence;
  }
}

void ConfidencePublisher::publish(const rcg::Buffer * buffer, uint32_t part, uint64_t pixelformat)
{
  if (pub.getNumSubscribers() > 0 && pixelformat == Confidence8) {
    // create image and initialize header

    std::shared_ptr<sensor_msgs::msg::Image> im = std::make_shared<sensor_msgs::msg::Image>();

    uint64_t time = buffer->getTimestampNS();

    im->header.stamp.sec = time / 1000000000ul;
    im->header.stamp.nanosec = time % 1000000000ul;
    im->header.frame_id = frame_id;

    // set image size

    im->width = static_cast<uint32_t>(buffer->getWidth(part));
    im->height = static_cast<uint32_t>(buffer->getHeight(part));

    // get pointer to image data in buffer

    size_t px = buffer->getXPadding(part);
    const uint8_t * ps = static_cast<const uint8_t *>(buffer->getBase(part));

    // convert image data

    im->encoding = sensor_msgs::image_encodings::TYPE_32FC1;
    im->is_bigendian = rcg::isHostBigEndian();
    im->step = im->width * sizeof(float);

    im->data.resize(im->step * im->height);
    float * pt = reinterpret_cast<float *>(&im->data[0]);

    float scale = 1.0f / 255.0f;

    for (uint32_t k = 0; k < im->height; k++) {
      for (uint32_t i = 0; i < im->width; i++) {
        *pt++ = scale * *ps++;
      }

      ps += px;
    }

    // publish message

    pub.publish(im);
  }
}

}  // namespace rc
