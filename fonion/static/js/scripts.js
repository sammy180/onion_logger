let currentFieldIndex = 0;
let scrollLabels = [];
let boxIds = [];

async function fetchScrollLabels() {
    try {
        const response = await fetch('/setup');
        const data = await response.json();
        scrollLabels = data.scroll_labels;
        boxIds = data.box_ids;
        console.log('Scroll labels and box IDs fetched:', scrollLabels, boxIds);
    } catch (error) {
        console.error('Error fetching scroll labels:', error);
    }
}

async function updateQuadrants() {
    if (scrollLabels.length === 0 || boxIds.length === 0) return;
    console.log('updateQuadrants has been run');
    
    try {
        const field = scrollLabels[currentFieldIndex];
        const response = await fetch(`/get_data?field=${field}`);
        const data = await response.json();
        console.log(`Box IDs: ${boxIds.join(', ')}`);
        console.log(`Current field: ${field}`);

        // First quadrant
        document.querySelector('.quadrant:nth-child(1) .title').textContent = `Box ${boxIds[0]}`;
        document.querySelector('.quadrant:nth-child(1) .parameter').textContent = `${field}:`;
        document.querySelector('.quadrant:nth-child(1) .value').textContent = `${data.box1}`;

        // Second quadrant
        document.querySelector('.quadrant:nth-child(2) .title').textContent = `Box ${boxIds[1]}`;
        document.querySelector('.quadrant:nth-child(2) .parameter').textContent = `${field}:`;
        document.querySelector('.quadrant:nth-child(2) .value').textContent = `${data.box2}`;

        // Third quadrant
        document.querySelector('.quadrant:nth-child(3) .title').textContent = `Box ${boxIds[2]}`;
        document.querySelector('.quadrant:nth-child(3) .parameter').textContent = `${field}:`;
        document.querySelector('.quadrant:nth-child(3) .value').textContent = `${data.box3}`;

        // Fourth quadrant
        document.querySelector('.quadrant:nth-child(4) .title').textContent = `Box ${boxIds[3]}`;
        document.querySelector('.quadrant:nth-child(4) .parameter').textContent = `${field}:`;
        document.querySelector('.quadrant:nth-child(4) .value').textContent = `${data.box4}`;

        console.log('updateQuadrants has been run');
    } catch (error) {
        console.error('Error fetching data:', error);
    }
}

function nextField() {
    currentFieldIndex = (currentFieldIndex + 1) % scrollLabels.length;
    updateQuadrants();
    console.log('nextField has been run');
}

function prevField() {
    currentFieldIndex = (currentFieldIndex - 1 + scrollLabels.length) % scrollLabels.length;
    updateQuadrants();
    console.log('prevField has been run');
}

// Fetch scroll labels and update quadrants when the page loads
window.onload = async () => {
    await fetchScrollLabels();
    updateQuadrants();
};