//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// Single point of authority for linking VS hierarchies with Project objects.
//
// The entire purpose of this class is to make it convenient to find a suitable
// Project to go along with a corresponding IVsHierarchy (VS project).
//

using Microsoft.VisualStudio.Shell.Interop;
using System.Collections.Generic;

namespace EpochVSIX.Parser
{
    public class ProjectMapper
    {
        private static ProjectMapper Instance = null;

        public static ProjectMapper GetInstance()
        {
            if (Instance == null)
                Instance = new ProjectMapper();

            return Instance;
        }



        private Dictionary<IVsHierarchy, Project> Mappings = new Dictionary<IVsHierarchy, Project>();

        public void Reset()
        {
            Mappings.Clear();
        }

        internal Project Map(IVsHierarchy project)
        {
            Project ret = null;

            if (Mappings.ContainsKey(project))
                ret = Mappings[project];
            else
            {
                ret = new Project(project);
                Mappings.Add(project, ret);
            }

            return ret;
        }
    }
}
