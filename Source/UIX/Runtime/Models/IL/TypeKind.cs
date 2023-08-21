namespace Studio.Models.IL
{
    /// <summary>
    /// Mirror of Cxx TypeKind, keep up to date
    /// </summary>
    public enum TypeKind
    {
        None,
        Bool,
        Void,
        Int,
        FP,
        Vector,
        Matrix,
        Pointer,
        Array,
        Texture,
        Buffer,
        Sampler,
        CBuffer,
        Function,
        Struct,
        Unexposed
    }
}