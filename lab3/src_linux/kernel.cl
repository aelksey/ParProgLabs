__kernel void find_min_triangle(
    __global const int* matrix,
    int rows,
    int cols,
    int tri_size,
    __global int* row_results,   // FIX: Changed from long to int to match Windows host dimensions
    __global int* col_results,
    __global ulong* sum_results) 
{
    // FIX: Mapping Dim 0 to Columns and Dim 1 to Rows speeds up AMD memory coalescing
    int c = get_global_id(0);
    int r = get_global_id(1);
    
    int padded_cols = get_global_size(0);
    int linear_idx = r * padded_cols + c;

    if (r >= 0 && r + tri_size <= rows && c >= 0 && c + tri_size <= cols) {
        ulong current_sum = 0;
        
        for (int i = 0; i < tri_size; i++) {
            ulong row_offset = (ulong)(r + i) * (ulong)cols;
            for (int j = 0; j <= i; j++) {
                current_sum += (ulong)matrix[row_offset + (ulong)(c + j)];
            }
        }
        
        row_results[linear_idx] = r;
        col_results[linear_idx] = c;
        sum_results[linear_idx] = current_sum;
    } else {
        sum_results[linear_idx] = 0xFFFFFFFFFFFFFFFFUL; // ULONG_MAX
        row_results[linear_idx] = -1;
        col_results[linear_idx] = -1;
    }
}
