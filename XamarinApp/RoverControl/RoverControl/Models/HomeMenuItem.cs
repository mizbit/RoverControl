﻿using System;
using System.Collections.Generic;
using System.Text;

namespace RoverControl.Models
{
    public enum MenuItemType
    {
        Control,
        Devices,
        About
    }
    public class HomeMenuItem
    {
        public MenuItemType Id { get; set; }

        public string Title { get; set; }
    }
}
