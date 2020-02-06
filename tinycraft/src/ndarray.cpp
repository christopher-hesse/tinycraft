/*

Multi-dimensional array class, does not own the data, but helps with indexing.

You can create windowed views of the data using .window() which allow out of
bounds access (including negative indexes) as long as they are still inside
of the underlying data array.

Additional checks are performed if running in debug mode.

*/

template <typename T, int NDIM>
struct NDArray {
    T *data = nullptr;
    int data_size = 0;

    int shape[NDIM] = {};
    int strides[NDIM] = {};
    int offsets[NDIM] = {};
    int full_shape[NDIM] = {};

    static NDArray<T, NDIM> create_from_data(T *data, const int *shape) {
        NDArray<T, NDIM> r;
        r.data = data;
        r.data_size = 1;
        for (int dim = 0; dim < NDIM; dim++) {
            fassert(shape[dim] > 0);
            r.shape[dim] = shape[dim];
            r.offsets[dim] = 0;
            r.full_shape[dim] = shape[dim];
            r.data_size *= shape[dim];
        }
        calculate_strides(r.shape, r.strides);
        return r;
    };

    static NDArray<T, NDIM> create_from_data(T *data, std::initializer_list<int> shape) {
        dassert(shape.size() == NDIM);
        return create_from_data(data, shape.begin());
    };

    static void calculate_strides(const int *shape, int *strides) {
        for (int dim = 0; dim < NDIM; dim++) {
            int stride = 1;
            for (int i = dim + 1; i < NDIM; i++) {
                stride *= shape[i];
            }
            strides[dim] = stride;
        }
    };

    NDArray<T, NDIM> window(const int *start, const int *view_shape) const {
        NDArray<T, NDIM> r;
        r.data = data;
        r.data_size = data_size;
        for (int dim = 0; dim < NDIM; dim++) {
            int size = view_shape[dim];
            fassert(0 < size && start[dim] + size <= shape[dim]);
            r.shape[dim] = size;
            r.strides[dim] = strides[dim];
            r.offsets[dim] = offsets[dim] + start[dim];
            fassert(0 <= r.offsets[dim] && r.offsets[dim] < full_shape[dim]);
            r.full_shape[dim] = full_shape[dim];
        }
        return r;
    };

    NDArray<T, NDIM> window(std::initializer_list<int> start, std::initializer_list<int> view_shape) const {
        dassert(start.size() == NDIM && view_shape.size() == NDIM);
        return window(start.begin(), view_shape.begin());
    };

    int size() const {
        int result = 1;
        for (int dim = 0; dim < NDIM; dim++) {
            result *= shape[dim];
        }
        return result;
    };

    void fill(T value) {
        int length = 1;
        for (int dim = 0; dim < NDIM; dim++) {
            length *= shape[dim];
        }
        int local_strides[NDIM];
        calculate_strides(shape, local_strides);
        // there could be a fast path here if the array is contiguous, that is, if local_strides == strides
        for (int i = 0; i < length; i++) {
            int indices[NDIM];
            for (int dim = 0; dim < NDIM; dim++) {
                indices[dim] = i / local_strides[dim] % shape[dim];
            }
            set(indices, value);
        }
    };

    int to_index(const int *indices) const {
        // returns the index into data
        int index = 0;
        for (int dim = 0; dim < NDIM; dim++) {
            index += strides[dim] * (offsets[dim] + indices[dim]);
        }
        return index;
    };

    bool contains(const int *indices) const {
        // check if the indices are contained in this window
        for (int dim = 0; dim < NDIM; dim++) {
            int index = indices[dim];
            if (!(0 <= index && index < shape[dim])) {
                return false;
            }
        }
        return true;
    };

    bool contains(std::initializer_list<int> indices) const {
        dassert(indices.size() == NDIM);
        return contains(indices.begin());
    };

    bool full_contains(const int *indices) const {
        // check if the indices are contained in the full array
        for (int dim = 0; dim < NDIM; dim++) {
            int index = offsets[dim] + indices[dim];
            if (!(0 <= index && index < full_shape[dim])) {
                return false;
            }
        }
        return true;
    };

    bool full_contains(std::initializer_list<int> indices) const {
        dassert(indices.size() == NDIM);
        return full_contains(indices.begin());
    };

    void set(const int *indices, T value) {
        int index = to_index(indices);
        dassert(full_contains(indices));
        data[index] = value;
    };

    void set(std::initializer_list<int> indices, T value) {
        dassert(indices.size() == NDIM);
        set(indices.begin(), value);
    };

    T get(const int *indices) const {
        int index = to_index(indices);
        dassert(full_contains(indices));
        return data[index];
    };

    T get(std::initializer_list<int> indices) const {
        dassert(indices.size() == NDIM);
        return get(indices.begin());
    };
};