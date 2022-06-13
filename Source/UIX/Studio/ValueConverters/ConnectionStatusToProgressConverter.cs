using System;
using System.Globalization;
using Avalonia;
using Avalonia.Data.Converters;
using Avalonia.Media;
using Studio.Models.Workspace;
using Studio.ViewModels;

namespace Studio.ValueConverters
{
    public class ConnectionStatusToProgressConverter : IValueConverter
    {
        /// <summary>
        /// Convert source data
        /// </summary>
        /// <param name="value">inbound value</param>
        /// <param name="targetType">expected type</param>
        /// <param name="parameter">originating parameter</param>
        /// <param name="language">culture</param>
        /// <returns>converted value</returns>
        /// <exception cref="NotSupportedException"></exception>
        public object Convert(object? value, Type targetType, object? parameter, CultureInfo language)
        {
            if (value is not ConnectionStatus status)
            {
                throw new NotSupportedException();
            }

            if (targetType == typeof(bool))
            {
                return ToEnabled(status);
            }
            else if(targetType == typeof(IBrush))
            {
                return ToColorBrush(status);
            }
            else if(targetType == typeof(string))
            {
                return ToMessage(status);
            }
            else
            {
                throw new NotSupportedException();
            }
        }

        /// <summary>
        /// Convert status to color brush
        /// </summary>
        /// <param name="status">connection status</param>
        /// <returns>color brush</returns>
        private SolidColorBrush ToColorBrush(ConnectionStatus status)
        {
            switch (status)
            {
                default:
                    return ResourceLocator.GetResource<SolidColorBrush>("DockApplicationAccentBrushLow");
                case ConnectionStatus.ResolveRejected:
                case ConnectionStatus.ApplicationRejected:
                    return ResourceLocator.GetResource<SolidColorBrush>("ErrorBrush");
                case ConnectionStatus.ResolveAccepted:
                case ConnectionStatus.ApplicationAccepted:
                    return ResourceLocator.GetResource<SolidColorBrush>("DockApplicationAccentBrushHigh");
            }
        }

        /// <summary>
        /// Convert status to enabled state
        /// </summary>
        /// <param name="status">connection status</param>
        /// <returns>state</returns>
        private bool ToEnabled(ConnectionStatus status)
        {
            switch (status)
            {
                default:
                    return false;
                case ConnectionStatus.Connecting:
                    return true;
            }
        }

        /// <summary>
        /// Convert status to message
        /// </summary>
        /// <param name="status">connection status</param>
        /// <returns>message string</returns>
        public string ToMessage(ConnectionStatus status)
        {
            switch (status)
            {
                default:
                    return "";
                case ConnectionStatus.ResolveRejected:
                    return "Endpoint rejected the application connection request";
                case ConnectionStatus.ResolveAccepted:
                    return "Endpoint accepted request, waiting for application server...";
                case ConnectionStatus.ApplicationRejected:
                    return "Endpoint application rejected request";
                case ConnectionStatus.ApplicationAccepted:
                    return "Endpoint application accepted request";
            }
        }
        
        /// <summary>
        /// Convert destination data
        /// </summary>
        /// <param name="value">inbound value</param>
        /// <param name="targetType">expected type</param>
        /// <param name="parameter">originating parameter</param>
        /// <param name="language">culture</param>
        /// <returns>converted value</returns>
        /// <exception cref="NotSupportedException"></exception>
        public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo language)
        {
            throw new NotSupportedException();
        }
    }
}