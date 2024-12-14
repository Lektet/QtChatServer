#ifndef UUIDHASH_H
#define UUIDHASH_H

#include <QHash>
#include <QUuid>
#include <functional>

namespace std {
  template<> struct hash<QUuid> {
    std::size_t operator()(const QUuid& id) const noexcept {
      return (size_t) qHash(id);
    }
  };
}

#endif // UUIDHASH_H
