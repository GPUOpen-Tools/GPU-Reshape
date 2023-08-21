namespace Studio.Models.IL
{
    /// <summary>
    /// Mirror of Cxx ConstantKind, keep up to date
    /// </summary>
    public enum ConstantKind
    {
        None,
        Undef,
        Bool,
        Int,
        FP,
        Struct,
        Null,
        Unexposed
    }
}