# Check if base directory parameter is provided
if ($args.Length -eq 0) {
    Write-Host "Error: Please provide the base directory where libraries should be managed."
    exit 1
}

# Assign base directory
$base_dir = $args[0]

# Check if requirements file exists
$requirements_file = "requirements_arduino.txt"
if (!(Test-Path $requirements_file)) {
    Write-Host "Error: Requirements file $requirements_file not found."
    exit 1
}

# Read repositories from requirements file into array
$repos = Get-Content $requirements_file

# Loop through each repository
foreach ($repo in $repos) {
    # Trim any leading/trailing whitespace
    $repo = $repo.Trim()

    # Extract repository name from URL
    $repo_name = [System.IO.Path]::GetFileNameWithoutExtension($repo)

    # Construct full path to library directory
    $library_dir = Join-Path -Path $base_dir -ChildPath $repo_name

    # Check if the directory exists
    if (Test-Path $library_dir -PathType Container) {
        Write-Host "Updating $repo_name"
        # Directory exists, so pull the latest changes
        Set-Location -Path $library_dir
        git pull
    } else {
        Write-Host "Cloning $repo_name"
        # Directory doesn't exist, so clone the repository
        git clone $repo $library_dir
    }
}