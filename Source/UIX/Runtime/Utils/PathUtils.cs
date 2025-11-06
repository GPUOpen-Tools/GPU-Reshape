using System;

namespace Studio.Utils;

public static class PathUtils
{
    /// <summary>
    /// Base module directory
    /// </summary>
    public static readonly string BaseDirectory = AppDomain.CurrentDomain.BaseDirectory;

    /// <summary>
    /// Standardize a path
    /// </summary>
    public static string StandardizePartialPath(string path)
    {
        // Standardize directory
        path = path.Replace("\\", "/");

        // Remove leading directories
        if (path[0] == '/')
        {
            path = path.Substring(1);
        }

        return path;
    }

    /// <summary>
    /// Remove the leading directory from a path
    /// </summary>
    public static string RemoveLeadingDirectory(string path)
    {
        // Otherwise, move the base up
        int separateIt = path.IndexOf('/');
        if (separateIt == -1)
        {
            return string.Empty;
        }
            
        return path.Substring(separateIt + 1);
    }
}
