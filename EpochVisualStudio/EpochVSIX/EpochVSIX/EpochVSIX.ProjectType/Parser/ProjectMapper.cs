using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.TextManager.Interop;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class ProjectMapper
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

        public Project Map(IVsHierarchy project)
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
