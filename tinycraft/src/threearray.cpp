template <typename T>
struct ThreeArray : public NDArray<T, 3> {
    ThreeArray(){};

    ThreeArray(const NDArray<T, 3> &arr) {
        // for some reason this doesn't work without this-> under gcc
        this->data = arr.data;
        this->data_size = arr.data_size;
        memcpy(this->shape, arr.shape, sizeof(this->shape));
        memcpy(this->strides, arr.strides, sizeof(this->strides));
        memcpy(this->offsets, arr.offsets, sizeof(this->offsets));
        memcpy(this->full_shape, arr.full_shape, sizeof(this->full_shape));
    };

    static ThreeArray<T> create_from_data(T *data, std::initializer_list<int> shape) {
        return ThreeArray<T>(NDArray<T, 3>::create_from_data(data, shape));
    };

    ThreeArray<T> window(std::initializer_list<int> start, std::initializer_list<int> view_shape) const {
        return ThreeArray<T>(NDArray<T, 3>::window(start, view_shape));
    };

    bool contains(const ivec3 indices) const {
        return NDArray<T, 3>::contains(&indices[0]);
    };

    T get(const ivec3 indices) const {
        return NDArray<T, 3>::get(&indices[0]);
    };

    void set(const ivec3 indices, T value) {
        NDArray<T, 3>::set(&indices[0], value);
    };
};