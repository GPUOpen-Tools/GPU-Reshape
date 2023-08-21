namespace Studio.Models.IL
{
    /// <summary>
    /// Mirror of Cxx AddressSpace, keep up to date
    /// </summary>
    public enum AddressSpace
    {
        Constant,
        Texture,
        Buffer,
        Function,
        Resource,
        GroupShared,
        RootConstant,
        Output,
        Unexposed
    }
}