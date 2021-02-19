#ifndef IDOCP_CONTACT_SEQUENCE_HPP_
#define IDOCP_CONTACT_SEQUENCE_HPP_ 

#include <deque>

#include "idocp/robot/robot.hpp"
#include "idocp/robot/contact_status.hpp"
#include "idocp/robot/impulse_status.hpp"
#include "idocp/hybrid/discrete_event.hpp"


namespace idocp {

///
/// @class ContactSequence
/// @brief The sequence of contact status and discrete events (impulse and lift). 
///
class ContactSequence {
public:
  ///
  /// @brief Constructor. 
  /// @param[in] robot Robot model. Must be initialized by URDF or XML.
  /// @param[in] max_num_events Maximum number of each discrete events 
  /// (impulse and lift). 
  ///
  ContactSequence(const Robot& robot, const int max_num_events);

  ///
  /// @brief Default constructor. 
  ///
  ContactSequence();

  ///
  /// @brief Destructor. 
  ///
  ~ContactSequence();

  ///
  /// @brief Default copy constructor. 
  ///
  ContactSequence(const ContactSequence&) = default;

  ///
  /// @brief Default copy assign operator. 
  ///
  ContactSequence& operator=(const ContactSequence&) = default;

  ///
  /// @brief Default move constructor. 
  ///
  ContactSequence(ContactSequence&&) noexcept = default;

  ///
  /// @brief Default move assign operator. 
  ///
  ContactSequence& operator=(ContactSequence&&) noexcept = default;

  ///
  /// @brief Set the contact status over all of the time stages uniformly. Also, 
  /// disable discrete events over all of the time stages.
  /// @param[in] contact_status Contact status.
  ///
  void setContactStatusUniformly(const ContactStatus& contact_status);

  ///
  /// @brief Push back the discrete event. Contact status after discrete event 
  /// is also appended according to discrete_event. 
  /// @param[in] discrete_event Discrete event.
  /// @param[in] event_time Time of the discrete event.
  ///
  void push_back(const DiscreteEvent& discrete_event, const double event_time);

  ///
  /// @brief Push back the discrete event that is automatically generated by
  /// last contact status of this object and contact_status.
  /// @param[in] contact_status Contact status.
  /// @param[in] event_time Time of the discrete event.
  ///
  void push_back(const ContactStatus& contact_status, const double event_time);

  ///
  /// @brief Pop back the discrete event. Contact status after discrete event 
  /// is also removed. 
  ///
  void pop_back();

  ///
  /// @brief Pop front the discrete event. Contact status before the front 
  /// discrete event is also removed. 
  ///
  void pop_front();

  ///
  /// @brief Update the instant of the impulse event. 
  /// @param[in] impulse_index Index of the impulse event. Must be non-negative
  /// and less than numImpulseEvents().
  /// @param[in] impulse_time Updated time.
  ///
  void updateImpulseTime(const int impulse_index, const double impulse_time);

  ///
  /// @brief Update the instant of the lfit event. 
  /// @param[in] lift_index Index of the lift event. Must be non-negative
  /// and less than numLiftEvents().
  /// @param[in] lift_time Updated time.
  ///
  void updateLiftTime(const int lift_index, const double lift_time);

  ///
  /// @brief Set the contact points to contact statsus with specified contact  
  /// phase. Also set the contact points of the discrete event just before the  
  /// contact phase.
  /// @param[in] contact_phase Contact phase.
  /// @param[in] contact_points Contact points.
  ///
  void setContactPoints(const int contact_phase, 
                        const std::vector<Eigen::Vector3d>& contact_points);

  ///
  /// @brief Returns number of impulse events. 
  /// @return Number of impulse events.
  ///
  int numImpulseEvents() const;

  ///
  /// @brief Returns number of lift events. 
  /// @return Number of lift events.
  ///
  int numLiftEvents() const;

  ///
  /// @brief Returns number of discrete events, i.e., sum of 
  /// numImpulseEvents() and numLiftEvents().
  /// @return Number of discrete events.
  ///
  int numDiscreteEvents() const;

  ///
  /// @brief Returns number of contact phases. 
  /// @return Number of contact phases.
  ///
  int numContactPhases() const;

  ///
  /// @brief Getter of the contact status. 
  /// @param[in] contact_phase Index of contact status phase.
  /// @return const reference to the contact status.
  ///
  const ContactStatus& contactStatus(const int contact_phase) const;

  ///
  /// @brief Getter of the impulse status. 
  /// @param[in] impulse_index Index of impulse event.
  /// @return const reference to the impulse status.
  ///
  const ImpulseStatus& impulseStatus(const int impulse_index) const;

  ///
  /// @brief Returns impulse event time. 
  /// @return Impulse event time.
  ///
  double impulseTime(const int impulse_index) const;

  ///
  /// @brief Returns lift event time. 
  /// @return Lift event time.
  ///
  double liftTime(const int lift_index) const;

private:
  int max_num_events_;
  ContactStatus default_contact_status_;
  std::deque<ContactStatus> contact_statuses_;
  std::deque<DiscreteEvent> impulse_events_;
  std::deque<int> event_index_impulse_, event_index_lift_;
  std::deque<double> event_time_, impulse_time_, lift_time_;
  std::deque<bool> is_impulse_event_;

  void clear_all();

};
 
} // namespace idocp 

#include "idocp/hybrid/contact_sequence.hxx"

#endif // IDOCP_CONTACT_SEQUENCE_HPP_ 