#ifndef _maps_Collector_hpp_
#define _maps_Collector_hpp_

#include <memory>
#include "SensorDataReceiver.hpp"

namespace maps {

class SensorDataReceiver;
class MapManager;
class BotWrapper;

class Collector {
public:
  struct DataListener {
    virtual void notify(const SensorDataReceiver::SensorData& iData) = 0;
  };

public:
  Collector();
  ~Collector();

  void setBotWrapper(const std::shared_ptr<BotWrapper>& iWrapper);
  bool start();
  bool stop();

  void addListener(const DataListener& iListener);
  void removeListener(const DataListener& iListener);

  std::shared_ptr<SensorDataReceiver> getDataReceiver() const;
  std::shared_ptr<MapManager> getMapManager() const;

  // convenience methods
  bool getLatestFullSweep(int64_t& oStartTime, int64_t& oEndTime) const;
  bool getLatestSwath(const float iMinAngle, const float iMaxAngle,
                      int64_t& oStartTime, int64_t& oEndTime) const;

protected:
  struct Helper;
  std::shared_ptr<Helper> mHelper;
};

}

#endif
