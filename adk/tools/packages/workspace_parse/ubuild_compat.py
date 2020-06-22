from .project import Project


def create_project(project):
    return Project(
        project.proj_projname,
        project.proj_fname,
        sdk=project.devkit_root,
        wsroot=project._get_wsroot_from_workspace_file(project.workspace_root, project.devkit_root)
    )
