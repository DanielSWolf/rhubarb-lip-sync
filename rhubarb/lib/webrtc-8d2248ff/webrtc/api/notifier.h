/*
 *  Copyright 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_API_NOTIFIER_H_
#define WEBRTC_API_NOTIFIER_H_

#include <list>

#include "webrtc/api/mediastreaminterface.h"
#include "webrtc/base/common.h"

namespace webrtc {

// Implement a template version of a notifier.
template <class T>
class Notifier : public T {
 public:
  Notifier() {
  }

  virtual void RegisterObserver(ObserverInterface* observer) {
    ASSERT(observer != NULL);
    observers_.push_back(observer);
  }

  virtual void UnregisterObserver(ObserverInterface* observer) {
    for (std::list<ObserverInterface*>::iterator it = observers_.begin();
         it != observers_.end(); it++) {
      if (*it == observer) {
        observers_.erase(it);
        break;
      }
    }
  }

  void FireOnChanged() {
    // Copy the list of observers to avoid a crash if the observer object
    // unregisters as a result of the OnChanged() call. If the same list is used
    // UnregisterObserver will affect the list make the iterator invalid.
    std::list<ObserverInterface*> observers = observers_;
    for (std::list<ObserverInterface*>::iterator it = observers.begin();
         it != observers.end(); ++it) {
      (*it)->OnChanged();
    }
  }

 protected:
  std::list<ObserverInterface*> observers_;
};

}  // namespace webrtc

#endif  // WEBRTC_API_NOTIFIER_H_
