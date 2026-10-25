from collections import OrderedDict
from collections.abc import Callable
from typing import Generic, TypeVar

Key = TypeVar("Key")
Val = TypeVar("Val")


class LRU(Generic[Key, Val]):
    def __init__(self, capacity: int) -> None:
        self.capacity = capacity
        self._items: OrderedDict[Key, Val] = OrderedDict()

    def __contains__(self, key: Key) -> bool:
        return key in self._items

    def __len__(self) -> int:
        return len(self._items)

    def get(self, key: Key, create: Callable[[], Val]) -> Val:
        item = self._items.get(key)
        if item is None:
            item = create()
            self._items[key] = item
        self._items.move_to_end(key)
        while len(self._items) > self.capacity:
            self._items.popitem(last=False)
        return item
